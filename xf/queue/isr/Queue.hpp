#pragma once

#include <bit>
#include <cstddef>
#include <optional>

#include <xf/isr/isr.hpp>

namespace xf::queue::isr {

/// An ISR-safe version of `Queue`, obtained by calling `Queue::to_isr()`.
/// Unlike it's ISR-unsafe counterpart, non-trivially copyable items are not allowed to avoid calling memory-allocation routines inside an ISR.
template<typename Item>
class Queue {
public:
    static_assert(std::is_trivially_copyable_v<Item>, "Items must be trivially copyable so that no allocation happens inside an ISR.");

    /// Constructs a new ISR-safe queue from the given handle.
    explicit Queue(QueueHandle_t);

    /// Tries pushing an item to the back of the queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xQueueSendFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/04-xQueueSendFromISR).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send(const Item&) const;

    /// Tries pushing an item to the back of the queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xQueueSendToBackFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/06-xQueueSendToBackFromISR).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send_to_back(const Item&) const;

    /// Tries pushing an item to the front of the queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xQueueSendToFrontFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/08-xQueueSendToFrontFromISR).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send_to_front(const Item&) const;
    struct ReceiveData {
        Item item;
        xf::isr::HigherPriorityTaskWoken higher_priority_task_woken;
    };

    /// Tries popping an item from the front of the queue and returns whether it was successful and, if so, whether a context switch needs to be performed alongside the item itself.
    /// Analogous to [`xQueueReceiveFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/10-xQueueReceiveFromISR).
    [[nodiscard]] std::optional<ReceiveData> receive();

    /// A version of `send_to_back()` that will write to the queue even if the queue is full, overwriting data that is already held in the queue, and returns whether a context switch needs to be performed.
    /// This function is intended for use with queues that have a length of one, meaning the queue is either empty or full.
    /// Analogous to [`xQueueOverwriteFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/12-xQueueOverwriteFromISR).
    [[nodiscard]] xf::isr::HigherPriorityTaskWoken overwrite(const Item&);

    /// Tries receiving an item from the front of the queue without popping it and returns whether it was successful and, if so, whether a context switch needs to be performed alongside the item itself.
    [[nodiscard]] std::optional<ReceiveData> peek();

    /// Obtains the number of messages stored in the queue.
    /// Analogous to [`uxQueueMessagesWaitingFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#uxqueuemessageswaitingfromisr).
    [[nodiscard]] size_t messages_waiting() const;

    /// Queries the queue to determine if it is empty.
    /// Analogous to [`xQueueIsQueueEmptyFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#xqueueisqueueemptyfromisr).
    [[nodiscard]] bool is_empty() const;

    /// Queries the queue to determine if it is full.
    /// Analogous to [`xQueueIsQueueFullFromISR`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#xqueueisqueuefullfromisr).
    [[nodiscard]] bool is_full() const;

private:
    std::optional<xf::isr::HigherPriorityTaskWoken> generic_send(const Item&, BaseType_t copy_position) const;

private:
    QueueHandle_t m_handle;
};

template<typename Item>
Queue<Item>::Queue(QueueHandle_t handle)
    : m_handle(handle) {
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send(const Item& item) const {
    return send_to_back(item);
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send_to_back(const Item& item) const {
    return generic_send(item, queueSEND_TO_BACK);
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send_to_front(const Item& item) const {
    return generic_send(item, queueSEND_TO_FRONT);
}

template<typename Item>
std::optional<typename Queue<Item>::ReceiveData> Queue<Item>::receive() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    alignas(Item) std::byte buffer[sizeof(Item)];
    if (xQueueReceiveFromISR(m_handle, &buffer, &higher_priority_task_woken) != pdTRUE)
        return std::nullopt;

    return { std::bit_cast<Item>(buffer), higher_priority_task_woken };
}

template<typename Item>
xf::isr::HigherPriorityTaskWoken Queue<Item>::overwrite(const Item& item) {
    // Overwrite is infallible
    return generic_send(item, queueOVERWRITE).value();
}

template<typename Item>
std::optional<typename Queue<Item>::ReceiveData> Queue<Item>::peek() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    alignas(Item) std::byte buffer[sizeof(Item)];
    if (xQueuePeekFromISR(m_handle, &buffer) != pdTRUE)
        return std::nullopt;

    return { std::bit_cast<Item>(buffer), higher_priority_task_woken };
}

template<typename Item>
size_t Queue<Item>::messages_waiting() const {
    return uxQueueMessagesWaitingFromISR(m_handle);
}

template<typename Item>
bool Queue<Item>::is_empty() const {
    return xQueueIsQueueEmptyFromISR(m_handle) == pdTRUE;
}

template<typename Item>
bool Queue<Item>::is_full() const {
    return xQueueIsQueueFullFromISR(m_handle) == pdTRUE;
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::generic_send(const Item& item, BaseType_t copy_position) const {
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueGenericSendFromISR(m_handle, &item, &higher_priority_task_woken, copy_position) != pdTRUE)
        return std::nullopt;
    return higher_priority_task_woken;
}

}
