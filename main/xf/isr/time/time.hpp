#pragma once

#include <xf/time/time.hpp>

namespace xf::isr::time {
struct Clock {
    using rep = TickType_t;
    using period = std::ratio<1, configTICK_RATE_HZ>;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<Clock>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return time_point { duration { xTaskGetTickCountFromISR() } };
    }
};

using Tick = Clock::time_point;
using Duration = Clock::duration;
using Milliseconds = std::chrono::duration<TickType_t, std::milli>;

inline Tick now() {
    return Clock::now();
}
}
