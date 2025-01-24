#pragma once

#include <functional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Notification.hpp"
#include <xf/fn.hpp>
#include <xf/time/time.hpp>

namespace xf::task {

using Handle = TaskHandle_t;

/// A high level, object-oriented abstraction over a dynamically allocated FreeRTOS task.
/// Declaring your own tasks is accomplished by deriving from this class (or `StaticTask`) and implementing the `run()` method. The `setup()` method can be optionally defined to implement initial one-shot configuration that require an active task context.
/// Tasks are not immediately valid upon construction and must be made so by calling the `create()` function before being used.
/// The array of task notifications is statically determined by the variadic template parameter of this class and each entry can be retrieved by index or type using the `Task::notification()` accessors.
/// See `StaticTask` for a statically-allocated version of this class.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/01-Tasks-overview for more information on how FreeRTOS tasks work and https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/01-Task-notifications for more information on how task notifications work.
template<std::derived_from<Notification>... Notifications>
class Task {
public:
    static_assert(sizeof...(Notifications) <= configTASK_NOTIFICATION_ARRAY_ENTRIES, "The number of notifications for a task must be less than or equal to `configTASK_NOTIFICATION_ARRAY_ENTRIES`");

    /// Destroys the task if it has been created, does nothing otherwise
    virtual ~Task();

    Task(Task&&) noexcept;
    Task& operator=(Task&&) noexcept;

    // There is no mechanism in FreeRTOS to copy a task.
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // - Task creation

    /// Create the task and add it to the list of tasks that are ready to run.
    /// Analogous to [`xTaskCreate`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/01-xTaskCreate).
    [[nodiscard]] bool create(const char* name, uint32_t stack_depth, UBaseType_t priority);

    /// Create the task without using a name and add it to the list of tasks that are ready to run.
    /// Analogous to  [`xTaskCreate`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/01-xTaskCreate).
    [[nodiscard]] bool create(uint32_t stack_depth, UBaseType_t priority);

#if ESP_PLATFORM

    /// Create the task and pin it to a particular core.
    /// Analogous to  [`xTaskCreatePinnedToCore`](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/system/freertos_additions.html#_CPPv423xTaskCreatePinnedToCore14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_tK10BaseType_t).
    [[nodiscard]] bool create_pinned_to_core(const char* name, uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id);

    /// Create the task without using a name and pin it to a particular core.
    /// Analogous to  [`xTaskCreatePinnedToCore`](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/system/freertos_additions.html#_CPPv423xTaskCreatePinnedToCore14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_tK10BaseType_t).
    [[nodiscard]] bool create_pinned_to_core(uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id);

#endif

    /// Remove the task from the RTOS kernels management. It will be removed from all ready, blocked, suspended and event lists.
    /// This function will be automatically called after `run` returns.
    /// Analogous to  [`vTaskDelete`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/03-vTaskDelete).
    void destroy();

    // - Task control

    /// Delay a task for a given duration.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to  [`vTaskDelay`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/01-vTaskDelay).
    template<typename Rep, typename Period>
    void delay(std::chrono::duration<Rep, Period> duration);

    /// Delay a task until a specified time. This function can be used by periodic tasks to ensure a constant execution frequency.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    /// Analogous to  [`xTaskDelayUntil`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/03-xTaskDelayUntil).
    template<typename Rep, typename Period>
    [[nodiscard]] time::Tick delay_until(time::Tick previous_wake_time, std::chrono::duration<Rep, Period> increment);

    /// Runs the given callback every `period` time.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    template<typename Rep, typename Period>
    void every(std::chrono::duration<Rep, Period> period, Fn<void()> auto&& callback);

    /// Runs the given callback every `period` time.
    /// The callback controls the flow of the loop by returning `xf::ControlFlow` values.
    /// Refer to the module-level `time` documentation to see how time is converted to FreeRTOS ticks.
    template<typename Rep, typename Period>
    void every(std::chrono::duration<Rep, Period> period, Fn<ControlFlow()> auto&& callback);

    /// Obtain the priority of the task.
    /// Analogous to  [`uxTaskPriorityGet`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/04-uxTaskPriorityGet).
    [[nodiscard]] UBaseType_t priority() const;

    /// Set the priority of the task.
    /// Analogous to  [`vTaskPrioritySet`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/05-vTaskPrioritySet).
    void set_priority(UBaseType_t);

    /// Suspend the task. When suspended a task will never get any microcontroller processing time, no matter what its priority.
    /// Analogous to  [`vTaskSuspend`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/06-vTaskSuspend).
    void suspend();

    /// Resumes the task from suspension.
    /// Analogous to  [`vTaskResume`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/07-vTaskResume).
    void resume();

    /// Forces a task to leave the Blocked state, and enter the Ready state, even if the event the task was in the Blocked state to wait for has not occurred, and any specified timeout has not expired.
    /// Analogous to  [`xTaskAbortDelay`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/02-Task-control/09-xTaskAbortDelay).
    void abort_delay();

    /// Obtains the notification at the specified index, which defaults to `tskDEFAULT_INDEX_TO_NOTIFY`.
    /// Notifications are specified through the class's variadic template parameter.
    template<size_t I = tskDEFAULT_INDEX_TO_NOTIFY>
    [[nodiscard]] auto& notification();

    /// Obtains the notification of the specified type.
    /// Notifications are specified through the class's variadic template parameter.
    template<typename Notification>
    [[nodiscard]] auto& notification();

    /// Obtains the raw `TaskHandle_t` behind the task.
    [[nodiscard]] Handle raw_handle() const;

protected:
    /// The first user-defined function that will be called when the task is created.
    /// Use this function to setup things that require an active task context or can't be done in the constructor.
    virtual void setup() { }

    /// The main user-defined function that will be called when the task is created.
    virtual void run() = 0;

    /// The raw FreeRTOS task.
    /// Calls, in order: `setup()` -> `run()` -> `destroy()`.
    static void task(void* raw_self);

protected:
    Task(size_t notification_index_do_not_override_default_value = 0);

    Handle m_handle { nullptr };

private:
    std::tuple<Notifications...> m_notifications;
};

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::Task(size_t notification_index)
    : m_notifications {
        Notifications { m_handle, notification_index++ }...
    } { }

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::~Task() {
    if (m_handle)
        destroy();
}

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::Task(Task&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_notifications(std::move(other.m_notifications)) {
}

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>& Task<Notifications...>::operator=(Task&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
        m_notifications = std::move(other.m_notifications);
    }
    return *this;
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create(const char* name, uint32_t stack_depth, UBaseType_t priority) {
    configASSERT(m_handle == nullptr);
    return xTaskCreate(task, name, stack_depth, this, priority, &m_handle) == pdPASS;
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create(uint32_t stack_depth, UBaseType_t priority) {
    return create(nullptr, stack_depth, priority);
}

#if ESP_PLATFORM

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create_pinned_to_core(const char* name, uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id) {
    configASSERT(m_handle == nullptr);
    return xTaskCreatePinnedToCore(task, name, stack_depth, this, priority, &m_handle, core_id) == pdPASS;
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create_pinned_to_core(uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id) {
    return create_pinned_to_core(nullptr, stack_depth, priority, core_id);
}

#endif

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::destroy() {
    configASSERT(m_handle);
    vTaskDelete(std::exchange(m_handle, nullptr));
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
void Task<Notifications...>::delay(std::chrono::duration<Rep, Period> duration) {
    vTaskDelay(time::to_raw_tick(duration));
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
time::Tick Task<Notifications...>::delay_until(time::Tick previous_wake_time, std::chrono::duration<Rep, Period> increment) {
    auto raw = previous_wake_time.time_since_epoch().count();
    vTaskDelayUntil(&raw, time::to_raw_tick(increment));
    return time::Tick { time::Duration { raw } };
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
void Task<Notifications...>::every(std::chrono::duration<Rep, Period> period, Fn<void()> auto&& callback) {
    auto time = time::now();
    while (true) {
        time = delay_until(time, period);
        std::invoke(callback);
    }
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
void Task<Notifications...>::every(std::chrono::duration<Rep, Period> period, Fn<ControlFlow()> auto&& callback) {
    auto time = time::now();
    while (true) {
        time = delay_until(time, period);
        if (std::invoke(callback) == ControlFlow::Break)
            break;
    }
}

template<std::derived_from<Notification>... Notifications>
UBaseType_t Task<Notifications...>::priority() const {
    return uxTaskPriorityGet(m_handle);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::set_priority(UBaseType_t p) {
    vTaskPrioritySet(m_handle, p);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::suspend() {
    vTaskSuspend(m_handle);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::resume() {
    vTaskResume(m_handle);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::abort_delay() {
    xTaskAbortDelay(m_handle);
}

template<std::derived_from<Notification>... Notifications>
template<size_t I>
auto& Task<Notifications...>::notification() {
    return std::get<I>(m_notifications);
}

template<std::derived_from<Notification>... Notifications>
template<typename Notification>
auto& Task<Notifications...>::notification() {
    return std::get<Notification>(m_notifications);
}

template<std::derived_from<Notification>... Notifications>
Handle Task<Notifications...>::raw_handle() const {
    return m_handle;
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::task(void* raw_self) {
    auto& self = *static_cast<Task*>(raw_self);

    self.setup();

    self.run();

    self.destroy();
}

}
