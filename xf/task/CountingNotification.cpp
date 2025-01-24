#include "CountingNotification.hpp"

namespace xf::task {

void CountingNotification::give() {
    (void)xTaskNotifyGiveIndexed(_handle, _index);
}

[[nodiscard]] uint32_t CountingNotification::await_take() {
    return take(time::FOREVER).value();
}

[[nodiscard]] uint32_t CountingNotification::await_fetch() {
    return fetch(time::FOREVER).value();
}

uint32_t CountingNotification::current_value() const {
    return ulTaskNotifyValueClearIndexed(_handle, _index, 0);
}

uint32_t CountingNotification::consume_value() {
    return ulTaskNotifyValueClearIndexed(_handle, _index, UINT32_MAX);
}

void CountingNotification::clear() {
    (void)ulTaskNotifyValueClearIndexed(_handle, _index, UINT32_MAX);
}

isr::CountingNotification CountingNotification::to_isr() {
    return { _handle, _index };
}

}
