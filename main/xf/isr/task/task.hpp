#pragma once

#include <xf/isr/isr.hpp>
#include <xf/task/task.hpp>

namespace xf::isr::task {
class CountingNotifier : public xf::task::Notifier {
public:
    CountingNotifier(xf::task::CountingNotifier& notifier)
        : Notifier(notifier) {
    }

    HigherPriorityTaskWoken give() {
        BaseType_t higher_priority_task_woken = pdFALSE;
        vTaskNotifyGiveIndexedFromISR(handle, index, &higher_priority_task_woken);
        return higher_priority_task_woken;
    }
};

class BinaryNotifier : public xf::task::Notifier {
public:
    BinaryNotifier(xf::task::BinaryNotifier& notifier)
        : Notifier(notifier) {
    }

    HigherPriorityTaskWoken set() {
        BaseType_t higher_priority_task_woken = pdFALSE;
        xTaskNotifyIndexedFromISR(handle, index, true, eSetValueWithOverwrite, &higher_priority_task_woken);
        return higher_priority_task_woken;
    }
};
}
