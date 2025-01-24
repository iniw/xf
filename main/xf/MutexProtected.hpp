#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <functional>
#include <optional>
#include <utility>
#include <variant>
#include <xf/time/time.hpp>
#include <xf/util/fn.hpp>

namespace xf {
template<typename T>
class MutexProtected {
public:
    MutexProtected(const MutexProtected&) = delete;
    MutexProtected(MutexProtected&&) = delete;
    MutexProtected& operator=(const MutexProtected&) = delete;
    MutexProtected& operator=(MutexProtected&&) = delete;

    template<typename... Args>
    explicit MutexProtected(Args&&... args)
    requires std::constructible_from<T, Args...>
        : m_value(std::forward<Args>(args)...) {
    }

    ~MutexProtected() {
        if (m_handle)
            destroy();
    }

    void create() {
        configASSERT(m_handle == nullptr);
        m_handle = xSemaphoreCreateMutexStatic(&m_static_semaphore);
    }

    void destroy() {
        configASSERT(m_handle);
        vSemaphoreDelete(std::exchange(m_handle, nullptr));
    }

    template<std::invocable<T&> FN, typename R = std::invoke_result_t<FN, T&>>
    R await_access(FN&& fn) {
        if constexpr (std::is_void_v<R>) {
            auto ret = access(std::forward<FN>(fn), time::FOREVER);
            configASSERT(ret.has_value());
            return;
        } else {
            return access(std::forward<FN>(fn), time::FOREVER).value();
        }
    }

    template<std::invocable<const T&> FN, typename R = std::invoke_result_t<FN, T&>>
    R await_access(FN&& fn) const {
        if constexpr (std::is_void_v<R>) {
            auto ret = access(std::forward<FN>(fn), time::FOREVER);
            configASSERT(ret.has_value());
            return;
        } else {
            return access(std::forward<FN>(fn), time::FOREVER).value();
        }
    }

    template<std::invocable<T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& fn, std::chrono::duration<Rep, Period> timeout) {
        if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
            return std::nullopt;

        if constexpr (std::is_void_v<R>) {
            std::invoke(std::forward<FN>(fn), m_value);

            auto give = xSemaphoreGive(m_handle);
            configASSERT(give == pdTRUE);

            return std::monostate {};
        } else {
            auto result = std::invoke(std::forward<FN>(fn), m_value);

            auto give = xSemaphoreGive(m_handle);
            configASSERT(give == pdTRUE);

            return result;
        }
    }

    template<std::invocable<const T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& fn, std::chrono::duration<Rep, Period> timeout) const {
        if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
            return std::nullopt;

        if constexpr (std::is_void_v<R>) {
            std::invoke(std::forward<FN>(fn), m_value);

            auto give = xSemaphoreGive(m_handle);
            configASSERT(give == pdTRUE);

            return std::monostate {};
        } else {
            auto result = std::invoke(std::forward<FN>(fn), m_value);

            auto give = xSemaphoreGive(m_handle);
            configASSERT(give == pdTRUE);

            return result;
        }
    }

private:
    T m_value;

    SemaphoreHandle_t m_handle;
    StaticSemaphore_t m_static_semaphore;
};
}
