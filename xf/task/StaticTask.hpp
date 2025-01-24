#pragma once

#include "Task.hpp"

namespace xf::task {

/// A statically allocated version of `Task`.
/// The tasks's stack depth is set and underlying stack memory is allocated based on the `STACK_DEPTH` template parameter.
/// Refer to `Task`'s documentation for more information.
template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
class StaticTask : public Task<Notifications...> {
public:
    static_assert(STACK_DEPTH * sizeof(StackType_t) >= configMINIMAL_STACK_SIZE);

    /// Create the task and add it to the list of tasks that are ready to run.
    /// Analogous to [`xTaskCreateStatic`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/02-xTaskCreateStatic).
    void create(const char* name, UBaseType_t priority);

    /// Create the task without using a name and add it to the list of tasks that are ready to run.
    /// Analogous to  [`xTaskCreateStatic`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/01-Task-creation/02-xTaskCreateStatic).
    void create(UBaseType_t priority);

#if ESP_PLATFORM

    /// Create the task and pin it to a particular core.
    /// Analogous to  [`xTaskCreateStaticPinnedToCore`](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/system/freertos_additions.html#_CPPv429xTaskCreateStaticPinnedToCore14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC11StackType_tPC12StaticTask_tK10BaseType_t).
    void create_pinned_to_core(const char* name, UBaseType_t priority, BaseType_t core_id);

    /// Create the task without using a name and pin it to a particular core.
    /// Analogous to  [`xTaskCreateStaticPinnedToCore`](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/api-reference/system/freertos_additions.html#_CPPv429xTaskCreateStaticPinnedToCore14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC11StackType_tPC12StaticTask_tK10BaseType_t).
    void create_pinned_to_core(UBaseType_t priority, BaseType_t core_id);

#endif

private:
    // Hide the visibility of the `create` function from the base class since that has a stack depth parameter, which we take as a template argument.
    // Their replacements, which don't take the stack depth parameter, are declared above.
    using Task<Notifications...>::create;
    using Task<Notifications...>::create_pinned_to_core;

    StaticTask_t m_task_buffer;
    std::array<StackType_t, STACK_DEPTH> m_stack_buffer;
};

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create(const char* name, UBaseType_t priority) {
    configASSERT(this->m_handle == nullptr);
    this->m_handle = xTaskCreateStatic(Task<Notifications...>::task, name, STACK_DEPTH, this, priority, m_stack_buffer.data(), &m_task_buffer);
}

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create(UBaseType_t priority) {
    create(nullptr, priority);
}

#if ESP_PLATFORM

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create_pinned_to_core(const char* name, UBaseType_t priority, BaseType_t core_id) {
    configASSERT(this->m_handle == nullptr);
    this->m_handle = xTaskCreateStaticPinnedToCore(Task<Notifications...>::task, name, STACK_DEPTH, this, priority, m_stack_buffer.data(), &m_task_buffer, core_id);
}

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create_pinned_to_core(UBaseType_t priority, BaseType_t core_id) {
    create_pinned_to_core(nullptr, priority, core_id);
}

#endif

}
