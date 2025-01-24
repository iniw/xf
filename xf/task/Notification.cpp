#include "Notification.hpp"

namespace xf::task {

void Notification::clear_state() {
    xTaskNotifyStateClearIndexed(_handle, _index);
}

}
