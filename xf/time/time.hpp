#pragma once

#include <chrono>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//! A set of small functions and type aliases for representing FreeRTOS ticks, durations and timeouts using the `std::chrono` library.
//! The underlying configuration of the FreeRTOS scheduler is respected, meaning xf's `Clock` operates under a period of `configTICK_RATE_HZ` and represents time points and durations using `TickType_t`.
//! The main function in this module is `to_raw_tick`, which takes in an arbitrary `std::chrono::duration` and converts it into the native FreeRTOS `TickType_t`.
//! Every user-facing 'timeout' parameter in xf goes through this function. It first converts the given duration to a `TickType_t` backed duration with millisecond precision using `std::chrono::round` and then converts to a FreeRTOS tick using `pdMS_TO_TICKS`, meaning you get the closest possible representation of the original duration.
//! In the case where the given duration is too large and performing arithmetic on it would cause overflow, `portMAX_DELAY` is returned.

namespace xf::time {

/// An `std::chrono`-compatible abstraction over the FreeRTOS scheduler clock, implementing the C++11 "Clock" named requirement.
/// The representation is a `TickType_t` and the period is controlled by `configTICK_RATE_HZ`.
/// Refer to https://en.cppreference.com/w/cpp/named_req/Clock.html for more information.
struct Clock {
    using rep = TickType_t;
    using period = std::ratio<1, configTICK_RATE_HZ>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<Clock>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point { duration { xTaskGetTickCount() } };
    }
};

/// An absolute point in time representing the number of ticks since the scheduler had started, backed by the native FreeRTOS `TickType_t` and operating under a period of `configTICK_RATE_HZ`.
using Tick = Clock::time_point;
/// A `std::chrono::duration` instantiation, backed by the native FreeRTOS `TickType_t` and operating under a period of `configTICK_RATE_HZ`.
using Duration = Clock::duration;
/// A `std::chrono::milliseconds`-like type backed by the native FreeRTOS `TickType_t`.
using Milliseconds = std::chrono::duration<Clock::rep, std::milli>;

/// The maximum amount of time to wait for a function to finish, meaning you are willing to wait indefinitely.
constexpr auto FOREVER = Duration { portMAX_DELAY };
/// The minimum amount of time to wait for a function to finish, meaning you are willing to not wait any time and the calling task will never be blocked.
constexpr auto NO_WAIT = Duration { 0 };

/// Obtains the number of ticks passed since the start of the scheduler.
inline Tick now() {
    return Clock::now();
}

/// Converts an arbitrary `std::chrono::duration` to a FreeRTOS `TickType_t`.
/// The conversion is done by first rouding the duration to milliseconds using `std::chrono::round` then passing that to `pdMS_TO_TICKS`, meaning you get the closest possible representation of the given duration.
/// When the duration is longer than the maximum number of ticks supported by FreeRTOS, `portMAX_DELAY` is returned.
template<typename rep, typename period>
constexpr TickType_t to_raw_tick(std::chrono::duration<rep, period> duration) {
    if constexpr (std::same_as<Milliseconds, decltype(duration)>) {
        return pdMS_TO_TICKS(duration.count());
    } else {
        return duration >= FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(std::chrono::round<Milliseconds>(duration).count());
    }
}

/// An optimzed version of `to_raw_tick` that, by receiving a duration of the same underlying period and representation of the FreeRTOS config, can just return it's `count()`.
constexpr TickType_t to_raw_tick(Duration duration) {
    return duration.count();
}

}
