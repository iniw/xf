#pragma once

#include <chrono>
#include <variant>
#include <xf/Queue.hpp>

namespace event {
struct ChangeTimeout {
    std::chrono::milliseconds new_timeout;
};

struct Report {
    enum class Type {
        Normal,
        Weird,
    };

    Type type;
};

using Event = std::variant<ChangeTimeout, Report>;
using Queue = xf::Queue<Event, 5>;
}
