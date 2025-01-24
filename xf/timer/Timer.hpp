#pragma once

#include <tuple>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include "isr/Timer.hpp"
#include <xf/isr/isr.hpp>
#include <xf/time/time.hpp>

namespace xf::timer {

/// Describes the behavior of a timer after it has been fired.
enum class Mode {
    /// Restart after firing using the configured period.
    Repeating,
    /// Fire just once but remain valid.
    /// Can be started again using `start()/await_start()`.
    SingleShot,
    /// Fire just once and destroy itself after the user-provided callback returns.
    /// Can be made valid again by calling `create()` but only after `is_active()` returns false.
    SelfDestructive,
};

using Handle = TimerHandle_t;

/// A high level abstraction over a FreeRTOS timer that supports injection of extra context variables.
/// The underlying timer is always statically allocated.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/05-Software-timers/01-Software-timers for more information on how FreeRTOS timers work.
template<typename... Ctx>
class Timer {
public:
    using Callback = void (*)(Ctx...);

    /// Constructs a new timer using the provided mode, callback and context.
    /// The timer is not yet valid and must be made so by calling the `create()` function before being used.
    Timer(Mode, Callback, Ctx...);

    /// Destroys the timer if it has been created, does nothing otherwise.
    ~Timer();

    Timer(Timer&&) noexcept;
    Timer& operator=(Timer&&) noexcept;

    // There is no mechanism in FreeRTOS to copy a timer
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    /// Creates the timer using the provided period.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerCreateStatic`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/22-xTimerCreateStatic).
    template<typename Rep, typename Period>
    void create(const char* name, std::chrono::duration<Rep, Period> period);

    /// Queries the timer to see if it is active or dormant.
    /// A timer will be dormant if:
    ///     1. It has been created but not started, or
    ///     2. It is an expired one-shot timer that has not been restarted.
    /// Timers are created in the dormant state. The `start()`, `reset()`, `change_period()` API functions and their ISR-safe equivalent can all be used to transition a timer into the active state.
    /// Analogous to [`xTimerIsTimerActive`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/03-xTimerIsTimerActive).
    [[nodiscard]] bool is_active() const;

    /// Waits indefinitely for the 'start' command to be sent to the timer command queue.
    /// Starting a timer ensures the timer is in the active state. If the timer is not stopped, deleted, or reset in the mean time, the callback function associated with the timer will get called 'n' ticks after  `start()` was called, where 'n' is the timers defined period.
    /// It is valid to call  `start()` before the RTOS scheduler has been started, but when this is done the timer will not actually start until the RTOS scheduler is started, and the timers expiry time will be relative to when the RTOS scheduler is started, not relative to when  `start()` was called.
    /// Analogous to [`xTimerStart`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/04-xTimerStart).
    void await_start();

    /// Waits up to `timeout` amount of time for the 'start' command to be sent to the timer command queue and returns whether it was successful.
    /// Starting a timer ensures the timer is in the active state. If the timer is not stopped, deleted, or reset in the mean time, the callback function associated with the timer will get called 'n' ticks after  `start()` was called, where 'n' is the timers defined period.
    /// It is valid to call  `start()` before the RTOS scheduler has been started, but when this is done the timer will not actually start until the RTOS scheduler is started, and the timers expiry time will be relative to when the RTOS scheduler is started, not relative to when  `start()` was called.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerStart`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/04-xTimerStart).
    template<typename Rep, typename Period>
    [[nodiscard]] bool start(std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the 'stop' command to be sent to the timer command queue.
    /// Stopping a timer ensures the it is not in the active state.
    /// Analogous to [`xTimerStop`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/05-xTimerStop).
    void await_stop();

    /// Waits up to `timeout` amount of time for the 'stop' command to be sent to the timer command queue and returns whether it was successful.
    /// Stopping a timer ensures the it is not in the active state.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerStop`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/05-xTimerStop).
    template<typename Rep, typename Period>
    [[nodiscard]] bool stop(std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the 'change period' command to be sent to the timer command queue with the provided value.
    /// This function can be called to change the period of an active or dormant state timer.Changing the period of a dormant timer will also start it.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerChangePeriod`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/06-xTimerChangePeriod).
    template<typename Rep, typename Period>
    void await_change_period(std::chrono::duration<Rep, Period> period);

    /// Waits up to `timeout` amount of time for the 'change period' command to be sent to the timer command queue with the provided value and returns whether it was successful.
    /// This function can be called to change the period of an active or dormant state timer.Changing the period of a dormant timer will also start it.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerChangePeriod`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/06-xTimerChangePeriod).
    template<typename Rep, typename Period, typename Rep2, typename Period2>
    [[nodiscard]] bool change_period(std::chrono::duration<Rep, Period> period, std::chrono::duration<Rep2, Period2> timeout);

    /// Waits indefinitely for the 'delete' command to be sent to the timer command queue.
    /// Note that for this timer to be re-created `is_active()` must return false, which may not happen immediately after destruction.
    /// Analogous to [`xTimerDelete`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/07-xTimerDelete).
    void await_destroy();

    /// Waits up to `timeout` amount of time for the 'delete 'command to be sent and returns whether it was successful.
    /// Note that for this timer to be re-created `is_active()` must return false, which may not happen immediately after destruction.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerDelete`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/07-xTimerDelete).
    template<typename Rep, typename Period>
    [[nodiscard]] bool destroy(std::chrono::duration<Rep, Period> timeout);

    /// Waits indefinitely for the 'reset' command to be sent to the timer command queue, which resets the timer's countdown if it was already started and starts it otherwise.
    /// Analogous to [`xTimerReset`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/08-xTimerReset).
    void await_reset();

    /// Waits up to `timeout` amount of time for the 'reset' command to be sent to the timer command queue and returns whether it was successful.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to [`xTimerReset`](https://freertos.org/Documentation/02-Kernel/04-API-references/11-Software-timers/08-xTimerReset).
    template<typename Rep, typename Period>
    [[nodiscard]] bool reset(std::chrono::duration<Rep, Period> timeout);

    /// Obtains the raw handle backing this timer.
    [[nodiscard]] Handle raw_handle();

    /// Creates an ISR-safe version of the timer.
    [[nodiscard]] isr::Timer<Ctx...> for_isr();

private:
    static void callback(TimerHandle_t);

    Handle m_handle { nullptr };
    StaticTimer_t m_static_timer;

    Mode m_mode;

    Callback m_callback;
    std::tuple<Ctx...> m_ctx;
};

template<typename... Ctx>
Timer<Ctx...>::Timer(Mode mode, Callback callback, Ctx... ctx)
    : m_mode(mode)
    , m_callback(callback)
    , m_ctx(ctx...) {
}

template<typename... Ctx>
Timer<Ctx...>::~Timer() {
    if (m_handle)
        await_destroy();
}

template<typename... Ctx>
Timer<Ctx...>::Timer(Timer&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_callback(std::exchange(other.m_callback, nullptr))
    , m_ctx(std::move(other.m_ctx))
    , m_mode(other.m_mode) {
}

template<typename... Ctx>
Timer<Ctx...>& Timer<Ctx...>::operator=(Timer&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            await_destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
        m_callback = std::exchange(other.m_callback, nullptr);
        m_ctx = std::move(other.m_ctx);
        m_mode = other.m_mode;
    }
    return *this;
}

template<typename... Ctx>
template<typename Rep, typename Period>
void Timer<Ctx...>::create(const char* name, std::chrono::duration<Rep, Period> period) {
    configASSERT(m_handle == nullptr);
    m_handle = xTimerCreateStatic(
        name,
        time::to_raw_tick(period),
        m_mode == Mode::Repeating,
        this,
        &Timer::callback,
        &m_static_timer);
}

template<typename... Ctx>
bool Timer<Ctx...>::is_active() const {
    return xTimerIsTimerActive(m_handle) != pdFALSE;
}

template<typename... Ctx>
void Timer<Ctx...>::await_start() {
    (void)start(time::FOREVER);
}

template<typename... Ctx>
template<typename Rep, typename Period>
bool Timer<Ctx...>::start(std::chrono::duration<Rep, Period> timeout) {
    return xTimerStart(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
void Timer<Ctx...>::await_stop() {
    (void)stop(time::FOREVER);
}

template<typename... Ctx>
template<typename Rep, typename Period>
bool Timer<Ctx...>::stop(std::chrono::duration<Rep, Period> timeout) {
    return xTimerStop(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
template<typename Rep, typename Period>
void Timer<Ctx...>::await_change_period(std::chrono::duration<Rep, Period> period) {
    (void)change_period(period, time::FOREVER);
}

template<typename... Ctx>
template<typename Rep, typename Period, typename Rep2, typename Period2>
bool Timer<Ctx...>::change_period(std::chrono::duration<Rep, Period> period, std::chrono::duration<Rep2, Period2> timeout) {
    return xTimerChangePeriod(m_handle, time::to_raw_tick(period), time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
void Timer<Ctx...>::await_destroy() {
    (void)destroy(time::FOREVER);
}

template<typename... Ctx>
template<typename Rep, typename Period>
bool Timer<Ctx...>::destroy(std::chrono::duration<Rep, Period> timeout) {
    if (xTimerDelete(m_handle, time::to_raw_tick(timeout)) == pdTRUE) {
        m_handle = nullptr;
        return true;
    } else {
        return false;
    }
}

template<typename... Ctx>
void Timer<Ctx...>::await_reset() {
    (void)reset(time::FOREVER);
}

template<typename... Ctx>
template<typename Rep, typename Period>
bool Timer<Ctx...>::reset(std::chrono::duration<Rep, Period> timeout) {
    return xTimerReset(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
Handle Timer<Ctx...>::raw_handle() {
    return m_handle;
}

template<typename... Ctx>
isr::Timer<Ctx...> Timer<Ctx...>::for_isr() {
    return { m_handle };
}

template<typename... Ctx>
void Timer<Ctx...>::callback(TimerHandle_t handle) {
    auto& self = *static_cast<Timer*>(pvTimerGetTimerID(handle));

    std::apply([&](Ctx&... ctx) { self.m_callback(ctx...); }, self.m_ctx);

    if (self.m_mode == Mode::SelfDestructive)
        self.await_destroy();
}

}
