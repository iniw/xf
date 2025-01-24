#include "BinaryNotification.hpp"

namespace xf::task {

void BinaryNotification::set() {
    (void)xTaskNotifyIndexed(_handle, _index, true, eSetValueWithOverwrite);
}

void BinaryNotification::await_get() {
    (void)get(time::FOREVER);
}

bool BinaryNotification::current_value() {
    return ulTaskNotifyValueClearIndexed(_handle, _index, 0);
}

isr::BinaryNotification BinaryNotification::to_isr() {
    return { _handle, _index };
}

}
