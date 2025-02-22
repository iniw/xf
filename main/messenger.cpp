#include "messenger.hpp"

using namespace std::literals;

void Messenger::run_impl() {
    auto coinflip = [] {
        return rand() % 2;
    };

    every(10s, [&] {
        if (coinflip()) {
            m_event_queue.await_send(event::Report {
                .type = coinflip() ? event::Report::Type::Normal : event::Report::Type::Weird,
            });
        } else {
            m_event_queue.await_send(event::ChangeTimeout {
                .new_timeout = coinflip() ? 5s : 20s,
            });
        }

        return xf::util::ControlFlow::Continue;
    });
}
