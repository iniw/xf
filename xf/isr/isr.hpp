#pragma once

#include <concepts>

#include <freertos/FreeRTOS.h>

namespace xf::isr {

/// A convenience alias to indicate whether an ISR routine can wake another task, indicating that the caller should perform a context switch, which can be done using `yield()`.
using HigherPriorityTaskWoken = bool;

/// When no arguments are passed, always performs a context switch.
/// When one or more arguments are passed, performs a context switch if all of the arguments evaluate to `true`.
void yield(std::convertible_to<HigherPriorityTaskWoken> auto&&... yield) {
    if (true and (HigherPriorityTaskWoken(yield) or ...))
        portYIELD_FROM_ISR();
}

}
