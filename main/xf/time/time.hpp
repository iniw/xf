#pragma once

#include <chrono>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace xf::time {
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

using Tick = Clock::time_point;
using Duration = Clock::duration;
using Milliseconds = std::chrono::duration<TickType_t, std::milli>;

constexpr auto FOREVER = Duration { portMAX_DELAY };
constexpr auto NO_WAIT = Duration { 0 };

inline Tick now() {
    return Clock::now();
}

template<typename rep, typename period>
constexpr TickType_t to_raw_tick(std::chrono::duration<rep, period> duration) {
    return duration >= FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(std::chrono::duration_cast<Milliseconds>(duration).count());
}
}
