#pragma once

#include <xf/isr/isr.hpp>

namespace xf::isr {
template<typename Item>
class Queue {
public:
    Queue() = default;

    static_assert(std::is_trivially_copyable_v<Item>, "Items must be trivially copyable so that no allocation happens inside an ISR.");

    template<size_t STATIC_SIZE>
    explicit Queue(xf::Queue<Item, STATIC_SIZE>& queue)
        : m_handle(queue.raw_handle()) {
    }

    [[nodiscard]] std::optional<HigherPriorityTaskWoken> send(const Item& item) const {
        return send_to_back(item);
    }

    [[nodiscard]] std::optional<HigherPriorityTaskWoken> send_to_back(const Item& item) const {
        return generic_send(item, queueSEND_TO_BACK);
    }

    [[nodiscard]] std::optional<HigherPriorityTaskWoken> send_to_front(const Item& item) const {
        return generic_send(item, queueSEND_TO_FRONT);
    }

    [[nodiscard]] HigherPriorityTaskWoken overwrite(const Item& item) {
        // Overwrite is infallible
        return generic_send(item, queueOVERWRITE).value();
    }

    struct ReceiveData {
        Item item;
        HigherPriorityTaskWoken higher_priority_task_woken;
    };

    [[nodiscard]] std::optional<ReceiveData> receive() {
        BaseType_t higher_priority_task_woken = pdFALSE;
        alignas(Item) std::byte buffer[sizeof(Item)];
        if (xQueueReceiveFromISR(m_handle, &buffer, &higher_priority_task_woken) != pdTRUE)
            return std::nullopt;

        return { *std::launder(reinterpret_cast<Item*>(&buffer)), higher_priority_task_woken };
    }

    [[nodiscard]] std::optional<ReceiveData> peek() {
        BaseType_t higher_priority_task_woken = pdFALSE;
        alignas(Item) std::byte buffer[sizeof(Item)];
        if (xQueuePeekFromISR(m_handle, &buffer) != pdTRUE)
            return std::nullopt;

        return { *std::launder(reinterpret_cast<Item*>(&buffer)), higher_priority_task_woken };
    }

    [[nodiscard]] bool is_empty() const {
        return xQueueIsQueueEmptyFromISR(m_handle) == pdTRUE;
    }

    [[nodiscard]] bool is_full() const {
        return xQueueIsQueueFullFromISR(m_handle) == pdTRUE;
    }

    [[nodiscard]] size_t messages_waiting() const {
        return uxQueueMessagesWaitingFromISR(m_handle);
    }

private:
    std::optional<HigherPriorityTaskWoken> generic_send(const Item& item, BaseType_t copy_position) const {
        BaseType_t higher_priority_task_woken = pdFALSE;
        if (xQueueGenericSendFromISR(m_handle, &item, &higher_priority_task_woken, copy_position) != pdTRUE)
            return std::nullopt;
        return higher_priority_task_woken;
    }

private:
    const QueueHandle_t& m_handle;
};
}
