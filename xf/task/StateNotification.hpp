
#pragma once

#include <chrono>
#include <cstring>

#include "Notification.hpp"
#include "isr/StateNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

/// A specialized task notification that stores a strongly-typed object.
/// To preserve memory and object safety, the type must be less than or equal to 32 bits in size to fit in a FreeRTOS notification slot and be trivially copyable.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/05-As-mailbox for more information on the pattern of using task notifications as a lightweight mailbox/queue.
template<typename T>
struct StateNotification : Notification {
    static_assert(sizeof(T) <= sizeof(uint32_t) and std::is_trivially_copyable_v<T>,
        "Type can't be stored in a FreeRTOS notification.");

    /// Overrides the state in this notification and marks it as pending.
    void set(T state);

    /// Waits indefinitely until the notification is pending and then retrieves the current state.
    [[nodiscard]] T await_get();

    /// Waits up to `timeout` amount of time for the notification to be pending and then retrieves the current state, otherwise returns `std::nullopt`.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<T> get(std::chrono::duration<Rep, Period> timeout);

    /// Creates an ISR-safe version of the notification.
    [[nodiscard]] isr::StateNotification<T> to_isr();
};

template<typename T>
void StateNotification<T>::set(T state) {
    uint32_t raw_value = 0;
    std::memcpy(&raw_value, &state, sizeof(T));
    xTaskNotifyIndexed(_handle, _index, raw_value, eSetValueWithOverwrite);
}

template<typename T>
[[nodiscard]] T StateNotification<T>::await_get() {
    return get(time::FOREVER).value();
}

template<typename T>
template<typename Rep, typename Period>
[[nodiscard]] std::optional<T> StateNotification<T>::get(std::chrono::duration<Rep, Period> timeout) {
    uint32_t raw_value = 0;
    const auto received = xTaskNotifyWaitIndexed(_index, 0, UINT32_MAX, &raw_value, time::to_raw_tick(timeout));
    if (received == pdFALSE)
        return std::nullopt;

    alignas(T) std::byte buffer[sizeof(T)];
    std::memcpy(buffer, &raw_value, sizeof(T));
    return std::bit_cast<T>(buffer);
}

template<typename T>
[[nodiscard]] isr::StateNotification<T> StateNotification<T>::to_isr() {
    return { _handle, _index };
}

}
