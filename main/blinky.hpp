#pragma once

#include "event.hpp"
#include <xf/task/task.hpp>

class Blinky : public xf::task::StaticTask<Blinky, 2048> {
    XF_TASK;

    void setup_impl();
    void run_impl();

public:
    Blinky(event::Queue& event_queue)
        : m_event_queue(event_queue) { }

    event::Queue& m_event_queue;

    xf::time::Milliseconds m_blink_timeout { std::chrono::seconds { 10 } };
    bool m_led_state { false };
};
