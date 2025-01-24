#pragma once

#include "Notification.hpp"
#include <xf/isr/isr.hpp>

namespace xf::task::isr {

/// An ISR-safe version of `BinaryNotification`, obtained by calling `BinaryNotification::to_isr()`.
struct BinaryNotification : Notification {
    /// Overrides the state in this notification to `true` and marks it as pending.
    xf::isr::HigherPriorityTaskWoken set();
};

}
