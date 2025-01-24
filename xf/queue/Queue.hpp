#pragma once

#include <optional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "isr/Queue.hpp"
#include <xf/mem/mem.hpp>
#include <xf/time/time.hpp>

namespace xf::queue {

using Handle = QueueHandle_t;

/// A high level abstraction over a dynamically allocated FreeRTOS Queue that provides type and object safety.
/// Non-trivially copyable items are heap allocated when sent, stored in the underlying queue using a pointer and then free'd when receiving, which allows conveniently passing complex types like `std::vector` or `std::string` in a memory-safe manner. Even though move semantics are respected when possible this can be considered a performance footgun, so statically asserting that your message type is trivially copyable is recommended to avoid potential performance regressions.
/// See `StaticQueue` for a statically-allocated version of this class.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues for more information on how FreeRTOS queues work.
template<typename Item>
class Queue {
public:
    /// Constructs a new queue.
    /// The queue is not yet valid and must be made so by calling the `create()` function before being used.
    Queue() = default;

    /// Destroys the queue if it has been created, does nothing otherwise.
    ~Queue();

    Queue(Queue&&) noexcept;
    Queue& operator=(Queue&&) noexcept;

    // There is no mechanism in FreeRTOS to copy a queue
    Queue(const Queue&) = delete;
    Queue operator=(const Queue&) = delete;

    /// Creates the queue with the given length, which is the maximum number of items the queue can hold.
    /// Analogous to [`xQueueCreate`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/01-xQueueCreate).
    [[nodiscard]] bool create(size_t length);

    /// Delete a queue - freeing all the memory allocated for storing of items placed on the queue.
    /// Analogous to [`vQueueDelete`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#vqueuedelete).
    void destroy();

    /// Waits indefinitely for the item to be pushed to the back of the queue.
    /// Analogous to [`xQueueSend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/03-xQueueSend).
    void await_send(const Item&);

    /// Waits indefinitely for the item to be pushed to the back of the queue.
    /// Analogous to [`xQueueSend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/03-xQueueSend).
    void await_send(Item&&);

    /// Waits up to `timeout` amount of time for the item to be pushed to the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/03-xQueueSend).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send(const Item&, std::chrono::duration<Rep, Period> timeout);

    /// Waits up to `timeout` amount of time for the item to be pushed to the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/03-xQueueSend).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send(Item&&, std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the item to be pushed to the back of the queue.
    /// Analogous to [`xQueueSendToBack`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/05-xQueueSendToBack).
    void await_send_to_back(const Item&);

    /// Waits indefinitely for the item to be pushed to the back of the queue.
    /// Analogous to [`xQueueSendToBack`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/05-xQueueSendToBack).
    void await_send_to_back(Item&&);

    /// Waits up to `timeout` amount of time for the item to be pushed to the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSendToBack`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/05-xQueueSendToBack).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(const Item&, std::chrono::duration<Rep, Period> timeout);

    /// Waits up to `timeout` amount of time for the item to be pushed to the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSendToBack`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/05-xQueueSendToBack).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(Item&&, std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the item to be pushed to the front of the queue.
    /// Analogous to [`xQueueSendToFront`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/07-xQueueSendToFront).
    void await_send_to_front(const Item&);

    /// Waits indefinitely for the item to be pushed to the front of the queue.
    /// Analogous to [`xQueueSendToFront`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/07-xQueueSendToFront).
    void await_send_to_front(Item&&);

    /// Waits up to `timeout` amount of time for the item to be pushed to the front of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSendToFront`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/07-xQueueSendToFront).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(const Item&, std::chrono::duration<Rep, Period> timeout);

    /// Waits up to `timeout` amount of time for the item to be pushed to the front of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueSendToFront`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/07-xQueueSendToFront).
    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(Item&&, std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for an item to be popped from the front of the queue.
    /// Analogous to [`xQueueReceive`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/09-xQueueReceive).
    [[nodiscard]] Item await_receive() const;

    /// Waits up to `timeout` amount of time for the item to be popped from the back of the queue and returns whether it successfully did so.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xQueueReceive`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/09-xQueueReceive).
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> receive(std::chrono::duration<Rep, Period> timeout) const;

    /// A version of `send_to_back()` that will write to the queue even if the queue is full, overwriting data that is already held in the queue.
    /// This function is intended for use with queues that have a length of one, meaning the queue is either empty or full.
    /// Analogous to [`xQueueOverwrite`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/11-xQueueOverwrite).
    [[nodiscard]] bool overwrite(const Item&);

    /// A version of `send_to_back()` that will write to the queue even if the queue is full, overwriting data that is already held in the queue.
    /// This function is intended for use with queues that have a length of one, meaning the queue is either empty or full.
    /// Analogous to [`xQueueOverwrite`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/11-xQueueOverwrite).
    [[nodiscard]] bool overwrite(Item&&);

    /// Waits indefinitely for an item to be received from the front of the queue without popping it.
    /// Analogous to [`xQueuePeek`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/13-xQueuePeek).
    [[nodiscard]] Item await_peek() const;

    /// Waits up to `timeout` amount of time for the item to be received from the front of the queue without popping it. Returns the item on success and `std::nullopt` otherwise.
    /// Analogous to [`xQueuePeek`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/13-xQueuePeek).
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> peek(std::chrono::duration<Rep, Period> timeout) const;

    /// Obtains the number of messages stored in the queue.
    /// Analogous to [`uxQueueMessagesWaiting`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#uxqueuemessageswaiting).
    [[nodiscard]] size_t messages_waiting() const;

    /// Obtains the number of free spaces in the queue.
    /// Analogous to [`uxQueueSpacesAvailable`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#uxqueuespacesavailable).
    [[nodiscard]] size_t spaces_available() const;

    /// Resets a queue to its original empty state.
    /// Analogous to [`xQueueReset`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement#xqueuereset).
    void reset();

    /// Convenience function that reports whether the queue is empty or not. Equivalent to `messages_waiting() == 0`.
    [[nodiscard]] bool is_empty() const;

    /// Convenience function that reports whether the queue is full or not. Equivalent to `spaces_available() == 0`.
    [[nodiscard]] bool is_full() const;

    /// Obtains the raw handle backing this queue.
    [[nodiscard]] Handle raw_handle() const;

    /// Creates an ISR-safe version of the queue.
    [[nodiscard]] isr::Queue<Item> for_isr();

protected:
    Handle m_handle { nullptr };

    // Non-trivially copyable items are supported through an indirection backed by a heap allocation.
    using StoredItem = std::conditional_t<
        std::is_trivially_copyable_v<Item>,
        Item,
        std::add_pointer_t<Item>>;

private:
    template<typename T, typename Rep, typename Period>
    bool generic_send(T&& item, BaseType_t copy_position, std::chrono::duration<Rep, Period> timeout);
};

template<typename Item>
Queue<Item>::~Queue() {
    if (m_handle)
        destroy();
}

template<typename Item>
Queue<Item>::Queue(Queue&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {
}

template<typename Item>
Queue<Item>& Queue<Item>::operator=(Queue&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
}

template<typename Item>
[[nodiscard]] bool Queue<Item>::create(size_t length) {
    configASSERT(m_handle == nullptr);
    m_handle = xQueueCreate(length, sizeof(StoredItem));
    return m_handle != nullptr;
}

template<typename Item>
void Queue<Item>::destroy() {
    configASSERT(m_handle);
    vQueueDelete(std::exchange(m_handle, nullptr));
}

template<typename Item>
void Queue<Item>::await_send(const Item& item) {
    (void)send(item, time::FOREVER);
}

template<typename Item>
void Queue<Item>::await_send(Item&& item) {
    (void)send(std::move(item), time::FOREVER);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return send_to_back(item, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return send_to_back(std::move(item), timeout);
}

template<typename Item>
void Queue<Item>::await_send_to_back(const Item& item) {
    (void)send_to_back(item, time::FOREVER);
}

template<typename Item>
void Queue<Item>::await_send_to_back(Item&& item) {
    (void)send_to_back(std::move(item), time::FOREVER);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_back(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(item, queueSEND_TO_BACK, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_back(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(std::move(item), queueSEND_TO_BACK, timeout);
}

template<typename Item>
void Queue<Item>::await_send_to_front(const Item& item) {
    (void)send_to_front(item, time::FOREVER);
}

template<typename Item>
void Queue<Item>::await_send_to_front(Item&& item) {
    (void)send_to_front(std::move(item), time::FOREVER);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_front(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(item, queueSEND_TO_FRONT, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_front(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(std::move(item), queueSEND_TO_FRONT, timeout);
}

template<typename Item>
Item Queue<Item>::await_receive() const {
    return receive(time::FOREVER).value();
}

template<typename Item>
template<typename Rep, typename Period>
std::optional<Item> Queue<Item>::receive(std::chrono::duration<Rep, Period> timeout) const {
    alignas(StoredItem) std::byte buffer[sizeof(StoredItem)];
    if (xQueueReceive(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
        return std::nullopt;

    auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return stored_item;
    } else {
        auto result { std::move(*stored_item) };
        mem::destroy(stored_item);
        return result;
    }
}

template<typename Item>
bool Queue<Item>::overwrite(const Item& item) {
    return generic_send(item, queueOVERWRITE, time::FOREVER);
}

template<typename Item>
bool Queue<Item>::overwrite(Item&& item) {
    return generic_send(std::move(item), queueOVERWRITE, time::FOREVER);
}

template<typename Item>
Item Queue<Item>::await_peek() const {
    return peek(time::FOREVER).value();
}

template<typename Item>
template<typename Rep, typename Period>
std::optional<Item> Queue<Item>::peek(std::chrono::duration<Rep, Period> timeout) const {
    alignas(StoredItem) std::byte buffer[sizeof(StoredItem)];
    if (xQueuePeek(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
        return std::nullopt;

    auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return stored_item;
    } else {
        return *stored_item;
    }
}

template<typename Item>
void Queue<Item>::reset() {
    xQueueReset(m_handle);
}

template<typename Item>
size_t Queue<Item>::messages_waiting() const {
    return uxQueueMessagesWaiting(m_handle);
}

template<typename Item>
size_t Queue<Item>::spaces_available() const {
    return uxQueueSpacesAvailable(m_handle);
}

template<typename Item>
bool Queue<Item>::is_empty() const {
    return messages_waiting() == 0;
}

template<typename Item>
bool Queue<Item>::is_full() const {
    return spaces_available() == 0;
}

template<typename Item>
Handle Queue<Item>::raw_handle() const {
    return m_handle;
}

template<typename Item>
isr::Queue<Item> Queue<Item>::for_isr() {
    return isr::Queue<Item> { m_handle };
}

template<typename Item>
template<typename T, typename Rep, typename Period>
bool Queue<Item>::generic_send(T&& item, BaseType_t copy_position, std::chrono::duration<Rep, Period> timeout) {
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return xQueueGenericSend(m_handle, &item, time::to_raw_tick(timeout), copy_position) == pdTRUE;
    } else {
        auto* new_item = mem::create<Item>(std::forward<T>(item));
        if (new_item == nullptr)
            return false;

        if (xQueueGenericSend(m_handle, &new_item, time::to_raw_tick(timeout), copy_position) == pdTRUE) {
            return true;
        } else {
            // Cleanup the allocation before returning failure
            mem::destroy(new_item);
            return false;
        }
    }
}

}
