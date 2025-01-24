#pragma once

#include <array>
#include <cstddef>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <optional>
#include <type_traits>
#include <utility>
#include <xf/time/time.hpp>
#include <xf/util/allocation.hpp>

namespace xf {
// Passing "0" as the size of the queue will make it dynamic
template<typename Item, size_t STATIC_SIZE = 0>
class Queue {
public:
    static_assert(STATIC_SIZE == 0 ? configSUPPORT_DYNAMIC_ALLOCATION : configSUPPORT_STATIC_ALLOCATION, "Unable to allocate queue - check FreeRTOS config");

    using Handle = QueueHandle_t;

    // Support non-trivially-copyable items if dynamic allocation is allowed
    using StoredItem = std::conditional_t<
        std::is_trivially_copyable_v<Item>,
        Item,
        std::enable_if_t<
            configSUPPORT_DYNAMIC_ALLOCATION,
            std::add_pointer_t<Item>>>;

    static constexpr auto ITEM_SIZE = sizeof(StoredItem);
    static constexpr auto IS_STATIC = STATIC_SIZE != 0;

    Queue() = default;

    Queue(Queue&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr)) {
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            if (m_handle)
                destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    ~Queue() {
        if (m_handle)
            destroy();
    }

    // There are no mechanism in FreeRTOS to copy a queue
    Queue(const Queue&) = delete;
    Queue operator=(const Queue&) = delete;

    // Static creation is infallible
    void create()
    requires(IS_STATIC)
    {
        configASSERT(m_handle == nullptr);
        m_handle = xQueueCreateStatic(STATIC_SIZE, ITEM_SIZE, m_static_storage.data(), &m_static_queue);
    }

    [[nodiscard]] bool create(size_t size)
    requires(not IS_STATIC)
    {
        configASSERT(m_handle == nullptr);
        m_handle = xQueueCreate(size, ITEM_SIZE);
        return m_handle != nullptr;
    }

    void destroy() {
        configASSERT(m_handle);
        vQueueDelete(std::exchange(m_handle, nullptr));
    }

    void await_send(const Item& item) {
        auto ret = send(item, time::FOREVER);
        configASSERT(ret == true);
    }

    void await_send(Item&& item) {
        auto ret = send(std::move(item), time::FOREVER);
        configASSERT(ret == true);
    }

    void reset_and_await_send(const Item& item) {
        reset();
        await_send(item);
    }

    void reset_and_await_send(Item&& item) {
        reset();
        await_send(std::move(item));
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send(const Item& item, std::chrono::duration<Rep, Period> timeout) {
        return send_to_back(item, timeout);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send(Item&& item, std::chrono::duration<Rep, Period> timeout) {
        return send_to_back(std::move(item), timeout);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(const Item& item, std::chrono::duration<Rep, Period> timeout) {
        return generic_send(item, queueSEND_TO_BACK, timeout);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(Item&& item, std::chrono::duration<Rep, Period> timeout) {
        return generic_send(std::move(item), queueSEND_TO_BACK, timeout);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(const Item& item, std::chrono::duration<Rep, Period> timeout) {
        return generic_send(item, queueSEND_TO_FRONT, timeout);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(Item&& item, std::chrono::duration<Rep, Period> timeout) {
        return generic_send(std::move(item), queueSEND_TO_FRONT, timeout);
    }

    [[nodiscard]] bool overwrite(const Item& item) {
        static_assert(STATIC_SIZE == 1 || STATIC_SIZE == 0, "Overwrite must only be used with queues of size 1");
        return generic_send(item, queueOVERWRITE, time::FOREVER);
    }

    [[nodiscard]] bool overwrite(Item&& item) {
        static_assert(STATIC_SIZE == 1 || STATIC_SIZE == 0, "Overwrite must only be used with queues of size 1");
        return generic_send(std::move(item), queueOVERWRITE, time::FOREVER);
    }

    [[nodiscard]] Item await_receive() const {
        return receive(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> receive(std::chrono::duration<Rep, Period> timeout) const {
        alignas(StoredItem) std::byte buffer[ITEM_SIZE];
        if (xQueueReceive(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
            return std::nullopt;

        auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
        if constexpr (std::is_trivially_copyable_v<Item>) {
            return stored_item;
        } else {
            auto result { std::move(*stored_item) };
            util::destroy(stored_item);
            return result;
        }
    }

    [[nodiscard]] Item await_peek() const {
        return peek(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> peek(std::chrono::duration<Rep, Period> timeout) const {
        alignas(StoredItem) std::byte buffer[ITEM_SIZE];
        if (xQueuePeek(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
            return std::nullopt;

        auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
        if constexpr (std::is_trivially_copyable_v<Item>) {
            return stored_item;
        } else {
            return *stored_item;
        }
    }

    void reset() {
        xQueueReset(m_handle);
    }

    [[nodiscard]] const Handle& raw_handle() const {
        return m_handle;
    }

    [[nodiscard]] size_t messages_waiting() const {
        return uxQueueMessagesWaiting(m_handle);
    }

    [[nodiscard]] size_t spaces_available() const {
        return uxQueueSpacesAvailable(m_handle);
    }

    [[nodiscard]] bool is_empty() const {
        return messages_waiting() == 0;
    }

    [[nodiscard]] bool is_full() const {
        return spaces_available() == 0;
    }

private:
    template<typename T, typename Rep, typename Period>
    bool generic_send(T&& item, BaseType_t copy_position, std::chrono::duration<Rep, Period> timeout) {
        if constexpr (std::is_trivially_copyable_v<Item>) {
            return xQueueGenericSend(m_handle, &item, time::to_raw_tick(timeout), copy_position) == pdTRUE;
        } else {
            auto* new_item = util::create<Item>(std::forward<T>(item));
            if (new_item == nullptr)
                return false;

            if (xQueueGenericSend(m_handle, &new_item, time::to_raw_tick(timeout), copy_position) == pdTRUE) {
                return true;
            } else {
                // Cleanup the allocation before returning failure
                util::destroy(new_item);
                return false;
            }
        }
    }

    Handle m_handle { nullptr };

    struct Empty1 { };
    struct Empty2 { };

    // These don't take up any space if the queue is dynamic
    [[no_unique_address]] std::conditional_t<IS_STATIC, StaticQueue_t, Empty1> m_static_queue;
    [[no_unique_address]] std::conditional_t<IS_STATIC, std::array<std::uint8_t, STATIC_SIZE * ITEM_SIZE>, Empty2> m_static_storage;
};
}
