#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace xf::task {

/// The base class representing a task notification.
/// This class does very little by itself, refer to it's superclasses for more functionality.
/// See https://freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Direct-to-task-notifications/01-Task-notifications for more information on how FreeRTOS task notifications work.
struct Notification {
    /// Sets the state of this notification to 'not pending'.
    /// Analogous to [`xTaskNotifyStateClearIndexed`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/05-Direct-to-task-notifications/09-xTaskNotifyStateClear)
    void clear_state();

    const TaskHandle_t& _handle;
    UBaseType_t _index;
};

}
