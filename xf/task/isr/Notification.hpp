#pragma once

#include <freertos/FreeRTOS.h>

namespace xf::task::isr {

/// An "ISR"'d version of the `Notification` type. Does nothing, serves only as a baseline to the other ISR-safe notifications.
struct Notification {
    TaskHandle_t _handle;
    size_t _index;
};

}
