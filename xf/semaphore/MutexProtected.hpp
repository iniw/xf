#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <xf/time/time.hpp>

namespace xf::semaphore {

using Handle = SemaphoreHandle_t;

/// An object that is protected by a mutex and can only be accessed through it.
/// The underlying semaphore is always statically allocated.
/// See https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/04-Mutexes for more information on how FreeRTOS mutexes work.
template<typename T>
class MutexProtected {
public:
    /// Constructs a new mutex-protected objec using the provided arguments.
    /// The object is not yet valid and must be made so by calling the `create()` function before being used.
    template<typename... Args>
    requires std::constructible_from<T, Args...>
    explicit MutexProtected(Args&&...);

    /// Destroys the underlying object and the mutex, if it has been created.
    ~MutexProtected();

    // `MutexProtected` objects are purposefully pinned in place after construction, to avoid memory safety issues and bad practices.
    MutexProtected(MutexProtected&&) = delete;
    MutexProtected& operator=(MutexProtected&&) = delete;
    MutexProtected(const MutexProtected&) = delete;
    MutexProtected& operator=(const MutexProtected&) = delete;

    /// Creates the underlying mutex, allowing access to the object.
    /// Analogous to [`xSemaphoreCreateMutexStatic`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/10-Semaphore-and-Mutexes/07-xSemaphoreCreateMutexStatic).
    void create();

    /// Destroys the underlying mutex.
    /// Analogous to [`vSemaphoreDelete`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/10-Semaphore-and-Mutexes/18-vSemaphoreDelete).
    void destroy();

    /// Waits indefinitely for access to the mutex to be granted then invokes the given callback with the object as a paramater and returns the return value of the callback.
    /// This method performs roughly the following steps: `xSemaphoreTake()` -> callback(object) -> `xSemaphoreGive()`.
    template<std::invocable<T&> FN, typename R = std::invoke_result_t<FN, T&>>
    R await_access(FN&& callback);

    /// Waits indefinitely for access to the mutex to be granted then invokes the given callback with the object as a paramater and returns the return value of the callback.
    /// This method performs roughly the following steps: `xSemaphoreTake()` -> callback(object) -> `xSemaphoreGive()`.
    template<std::invocable<const T&> FN, typename R = std::invoke_result_t<FN, const T&>>
    R await_access(FN&& callback) const;

    /// Waits up to `timeout` amount of time to be allowed access to the mutex and returns whether it was successful and, if so, the return value of the callback.
    template<std::invocable<T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& callback, std::chrono::duration<Rep, Period> timeout);

    /// Waits up to `timeout` amount of time to be allowed access to the mutex and returns whether it was successful and, if so, the return value of the callback.
    template<std::invocable<const T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, const T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& callback, std::chrono::duration<Rep, Period> timeout) const;

    /// Obtains the raw handle backing the semaphore.
    [[nodiscard]] Handle raw_handle();

private:
    T m_value;

    Handle m_handle;
    StaticSemaphore_t m_static_semaphore;
};

template<typename T>
template<typename... Args>
requires std::constructible_from<T, Args...>
MutexProtected<T>::MutexProtected(Args&&... args)
    : m_value(std::forward<Args>(args)...) {
}

template<typename T>
MutexProtected<T>::~MutexProtected() {
    if (m_handle)
        destroy();
}

template<typename T>
void MutexProtected<T>::create() {
    configASSERT(m_handle == nullptr);
    m_handle = xSemaphoreCreateMutexStatic(&m_static_semaphore);
}

template<typename T>
void MutexProtected<T>::destroy() {
    configASSERT(m_handle);
    vSemaphoreDelete(std::exchange(m_handle, nullptr));
}

template<typename T>
template<std::invocable<T&> FN, typename R>
R MutexProtected<T>::await_access(FN&& callback) {
    if constexpr (std::is_void_v<R>) {
        (void)access(std::forward<FN>(callback), time::FOREVER);
    } else {
        return access(std::forward<FN>(callback), time::FOREVER).value();
    }
}

template<typename T>
template<std::invocable<const T&> FN, typename R>
R MutexProtected<T>::await_access(FN&& callback) const {
    if constexpr (std::is_void_v<R>) {
        (void)access(std::forward<FN>(callback), time::FOREVER);
    } else {
        return access(std::forward<FN>(callback), time::FOREVER).value();
    }
}

template<typename T>
template<std::invocable<T&> FN, typename Rep, typename Period, typename R>
std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> MutexProtected<T>::access(FN&& callback, std::chrono::duration<Rep, Period> timeout) {
    if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
        return std::nullopt;

    if constexpr (std::is_void_v<R>) {
        std::invoke(std::forward<FN>(callback), m_value);

        xSemaphoreGive(m_handle);

        return std::monostate {};
    } else {
        auto result = std::invoke(std::forward<FN>(callback), m_value);

        xSemaphoreGive(m_handle);

        return result;
    }
}

template<typename T>
template<std::invocable<const T&> FN, typename Rep, typename Period, typename R>
std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> MutexProtected<T>::access(FN&& callback, std::chrono::duration<Rep, Period> timeout) const {
    if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
        return std::nullopt;

    if constexpr (std::is_void_v<R>) {
        std::invoke(std::forward<FN>(callback), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return std::monostate {};
    } else {
        auto result = std::invoke(std::forward<FN>(callback), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return result;
    }
}

template<typename T>
Handle MutexProtected<T>::raw_handle() {
    return m_handle;
}

}
