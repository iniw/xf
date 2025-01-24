#pragma once

#include <chrono>
#include <optional>

#include "Notification.hpp"
#include "isr/CountingNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

/// A specialized task notification that stores an unsigned 32 bit counter.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/03-As-counting-semaphore for more information on the pattern of using task notifications as a lightweight counting semaphore.
struct CountingNotification : Notification {
    /// Increment the counter and mark this notification as pending.
    /// Analogous to [`xTaskNotifyGive`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/05-Direct-to-task-notifications/01-xTaskNotifyGive).
    void give();

    /// Waits indefinitely for the notification to be pending then decrements the counter, returning the previous value.
    /// Analogous to [`ulTaskNotifyTake`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/05-Direct-to-task-notifications/03-ulTaskNotifyTake).
    [[nodiscard]] uint32_t await_take();

    /// Waits up to `timeout` amount of time for the notification to be pending then decrements the counter and obtains the previous value, otherwise returns `std::nullopt`.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to  [`ulTaskNotifyTake`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/05-Direct-to-task-notifications/03-ulTaskNotifyTake).
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> take(std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the notification to be pending then obtains the current value without decrementing it.
    [[nodiscard]] uint32_t await_fetch();

    /// Waits up to `timeout` amount of time for the notification to be pending then obtains the current value without decrementing it, otherwise returns `std::nullopt`.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> fetch(std::chrono::duration<Rep, Period> timeout);

    /// Obtains the current value of the counter, regardless if the notification is pending or not.
    uint32_t current_value() const;

    /// Obtains the current value of the counter and resets it to 0, regardless if the notification is pending or not.
    uint32_t consume_value();

    /// Resets the counter to 0, regardless if the notification pending or not.
    void clear();

    /// Creates an ISR-safe version of the notification.
    [[nodiscard]] isr::CountingNotification to_isr();
};

template<typename Rep, typename Period>
[[nodiscard]] std::optional<uint32_t> CountingNotification::take(std::chrono::duration<Rep, Period> timeout) {
    BaseType_t value = ulTaskNotifyTakeIndexed(_index, pdTRUE, time::to_raw_tick(timeout));
    if (value == pdFALSE)
        return std::nullopt;

    return value;
}

template<typename Rep, typename Period>
[[nodiscard]] std::optional<uint32_t> CountingNotification::fetch(std::chrono::duration<Rep, Period> timeout) {
    uint32_t result = 0;

    BaseType_t received = xTaskNotifyWaitIndexed(_index, 0, 0, &result, time::to_raw_tick(timeout));
    if (received == pdFALSE)
        return std::nullopt;

    return result;
}
}
