#pragma once

#include "event.hpp"
#include <xf/task/task.hpp>

class Messenger : public xf::task::StaticTask<Messenger, 2048> {
    XF_TASK;

    void run_impl();

public:
    Messenger(event::Queue& event_queue)
        : m_event_queue(event_queue) { }

private:
    event::Queue& m_event_queue;
};
