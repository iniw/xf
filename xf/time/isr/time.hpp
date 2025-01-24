#pragma once

#include <xf/time/time.hpp>

namespace xf::time::isr {

/// Obtains the number of ticks passed since the start of the scheduler.
inline Tick now() {
    return Tick { Duration { xTaskGetTickCountFromISR() } };
}

}
