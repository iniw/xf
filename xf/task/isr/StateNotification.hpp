
#pragma once

#include <cstring>

#include "Notification.hpp"
#include <xf/isr/isr.hpp>

namespace xf::task::isr {

/// An ISR-safe version of `StateNotification`, obtained by calling `StateNotification::to_isr()`.
template<typename T>
struct StateNotification : Notification {
    static_assert(sizeof(T) <= sizeof(uint32_t) and std::is_trivially_copyable_v<T>,
        "Type can't be stored in a FreeRTOS notification");

    /// Overrides the state in this notification and marks it as pending.
    xf::isr::HigherPriorityTaskWoken set(T state);
};

template<typename T>
xf::isr::HigherPriorityTaskWoken StateNotification<T>::set(T state) {
    uint32_t raw_value = 0;
    std::memcpy(&raw_value, &state, sizeof(T));

    BaseType_t higher_priority_task_woken = pdFALSE;
    xTaskNotifyIndexedFromISR(_handle, _index, raw_value, eSetValueWithOverwrite, &higher_priority_task_woken);
    return higher_priority_task_woken;
}

}
