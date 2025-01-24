#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <functional>
#include <optional>
#include <utility>
#include <xf/isr/isr.hpp>
#include <xf/time/time.hpp>
#include <xf/util/allocation.hpp>
#include <xf/util/fn.hpp>

namespace xf {
class Timer {
public:
    enum class Mode {
        Repetitive,
        SingleShot,
        SelfDestructive,
    };

    using Handle = TimerHandle_t;

    Timer(Mode mode = Mode::SingleShot)
        : m_mode(mode) {
    }

    template<util::Fn<void()> FN>
    Timer(FN&& callback, Mode autoreload = Mode::SingleShot)
        : m_mode(autoreload)
        , m_callback(std::forward<FN>(callback)) {
    }

    Timer(Timer&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr)) {
    }

    Timer& operator=(Timer&& other) noexcept {
        if (this != &other) {
            if (m_handle)
                await_destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    ~Timer() {
        if (m_handle)
            await_destroy();
    }

    // There are no mechanism in FreeRTOS to copy a timer
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    template<typename Rep, typename Period>
    void create(const char* id, std::chrono::duration<Rep, Period> period) {
        configASSERT(m_handle == nullptr);
        m_handle = xTimerCreateStatic(
            id,
            time::to_raw_tick(period),
            m_mode == Mode::Repetitive,
            this,
            &Timer::callback,
            &m_static_timer);
    }

    void await_start() {
        auto ret = start(time::FOREVER);
        configASSERT(ret == true);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool start(std::chrono::duration<Rep, Period> timeout) {
        return xTimerStart(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
    }

    [[nodiscard]] std::optional<isr::HigherPriorityTaskWoken> start_from_isr() {
        BaseType_t higher_priority_task_woken = false;
        if (xTimerStartFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
            return std::nullopt;

        return higher_priority_task_woken;
    }

    void await_stop() {
        auto ret = stop(time::FOREVER);
        configASSERT(ret == true);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool stop(std::chrono::duration<Rep, Period> timeout) {
        return xTimerStop(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
    }

    [[nodiscard]] std::optional<bool> stop_from_isr() {
        BaseType_t higher_priority_task_woken = false;
        if (xTimerStopFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
            return std::nullopt;

        return higher_priority_task_woken;
    }

    template<typename Rep, typename Period>
    void await_change_period(std::chrono::duration<Rep, Period> period) {
        auto ret = change_period(period, time::FOREVER);
        configASSERT(ret == true);
    }

    template<typename Rep, typename Period, typename Rep2, typename Period2>
    bool change_period(std::chrono::duration<Rep, Period> period, std::chrono::duration<Rep2, Period2> timeout) {
        return xTimerChangePeriod(m_handle, time::to_raw_tick(period), time::to_raw_tick(timeout)) == pdTRUE;
    }

    void await_destroy() {
        auto ret = destroy(time::FOREVER);
        configASSERT(ret == true);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool destroy(std::chrono::duration<Rep, Period> timeout) {
        if (xTimerDelete(m_handle, time::to_raw_tick(timeout)) == pdTRUE) {
            m_handle = nullptr;
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] bool is_active() const {
        return xTimerIsTimerActive(m_handle) != pdFALSE;
    }

    [[nodiscard]] bool is_valid() const {
        return m_handle != nullptr;
    }

    template<util::Fn<void()> FN>
    void set_callback(FN&& callback) {
        m_callback = std::forward<FN>(callback);
    }

private:
    static void callback(TimerHandle_t handle) {
        auto& self = *static_cast<Timer*>(pvTimerGetTimerID(handle));
        self.m_callback();
        if (self.m_mode == Mode::SelfDestructive)
            self.await_destroy();
    }

    Mode m_mode;
    std::function<void()> m_callback;

    Handle m_handle { nullptr };
    StaticTimer_t m_static_timer;
};
}
