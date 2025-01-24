#pragma once

#include <optional>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <xf/isr/isr.hpp>
#include <xf/time/time.hpp>

namespace xf::timer::isr {

/// An ISR-safe version of `Timer`, obtained by calling `Timer::to_isr()`.
template<typename... Ctx>
class Timer {
public:
    /// Constructs a new ISR-safe timer from the given handle.
    explicit Timer(TimerHandle_t);

    /// Tries sending the 'start' command to the timer command queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xTimerStartFromISR`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/09-xTimerStartFromISR.).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> start();

    /// Tries sending the 'stop' command to the timer command queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xTimerStopFromISR`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/10-xTimerStopFromISR).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> stop();

    /// Tries sending the 'change period' command to the timer command queue with the provided value and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xTimerChangePeriodFromISR`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/11-xTimerChangePeriodFromISR).
    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> change_period(std::chrono::duration<Rep, Period> period);

    /// Tries sending the 'reset' command to the timer command queue and returns whether it was successful and, if so, whether a context switch needs to be performed.
    /// Analogous to [`xTimerResetFromISR`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/12-xTimerResetFromISR).
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> reset();

private:
    TimerHandle_t m_handle { nullptr };
};

template<typename... Ctx>
Timer<Ctx...>::Timer(TimerHandle_t handle)
    : m_handle(handle) {
}

template<typename... Ctx>
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::start() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerStartFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::stop() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerStopFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
template<typename Rep, typename Period>
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::change_period(std::chrono::duration<Rep, Period> period) {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerChangePeriodFromISR(m_handle, time::to_raw_tick(period), time::to_raw_tick(period)) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::reset() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerResetFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

}
