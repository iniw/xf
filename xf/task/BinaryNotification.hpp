
#pragma once

#include <chrono>

#include "Notification.hpp"
#include "isr/BinaryNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

/// A specialized task notification that stores a boolean/binary state.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/02-As-binary-semaphore for more information on the pattern of using task notifications as a lightweight binary semaphore.
struct BinaryNotification : Notification {
    /// Overrides the state in this notification to `true` and marks it as pending.
    void set();

    /// Waits indefinitely for the notification to be pending and then consumes the state, setting it to `false`.
    void await_get();

    /// Waits up to `timeout` amount of time for the notification to be pending and then retrieves the current state, otherwise returns `std::nullopt`.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    template<typename Rep, typename Period>
    [[nodiscard]] bool get(std::chrono::duration<Rep, Period> timeout);

    /// Obtains the current state of the notification, regardless if it's pending or not.
    [[nodiscard]] bool current_value();

    /// Creates an ISR-safe version of the notification.
    [[nodiscard]] isr::BinaryNotification to_isr();
};

template<typename Rep, typename Period>
[[nodiscard]] bool BinaryNotification::get(std::chrono::duration<Rep, Period> timeout) {
    return static_cast<bool>(xTaskNotifyWaitIndexed(_index, 0, UINT32_MAX, nullptr, time::to_raw_tick(timeout)));
}

}
