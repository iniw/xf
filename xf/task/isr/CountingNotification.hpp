#pragma once

#include "Notification.hpp"
#include <xf/isr/isr.hpp>

namespace xf::task::isr {

/// An ISR-safe version of `CountingNofication`, obtained by calling `CountingNofication::to_isr()`.
struct CountingNotification : Notification {
    /// Increment the counter and mark this notification as pending.
    /// Analogous to [`vTaskNotifyGiveFromIsr`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/05-Direct-to-task-notifications/02-vTaskNotifyGiveFromISR)
    xf::isr::HigherPriorityTaskWoken give();
};

}
