#include "BinaryNotification.hpp"

namespace xf::task::isr {

xf::isr::HigherPriorityTaskWoken BinaryNotification::set() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    xTaskNotifyIndexedFromISR(_handle, _index, true, eSetValueWithOverwrite, &higher_priority_task_woken);
    return higher_priority_task_woken;
}

}
