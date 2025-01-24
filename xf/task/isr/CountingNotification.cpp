#include "CountingNotification.hpp"

namespace xf::task::isr {

xf::isr::HigherPriorityTaskWoken CountingNotification::give() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveIndexedFromISR(_handle, _index, &higher_priority_task_woken);
    return higher_priority_task_woken;
}

}
