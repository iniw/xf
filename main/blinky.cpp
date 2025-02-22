#include "blinky.hpp"
#include <esp_log.h>

void Blinky::setup_impl() {
    ESP_LOGI("Blinky", "Hello :)");
}

void Blinky::run_impl() {
    const auto start = xf::time::now();

    while (true) {
        if (auto received = m_event_queue.receive(m_blink_timeout)) {
            auto& event = received.value();
            if (auto* change_timeout = std::get_if<event::ChangeTimeout>(&event)) {
                m_blink_timeout = change_timeout->new_timeout;
            } else if (auto* report = std::get_if<event::Report>(&event)) {
                switch (report->type) {
                case event::Report::Type::Normal:
                    ESP_LOGI("Blinky", "Led is currently %s with a timeout set to %lums.", m_led_state ? "on" : "off", m_blink_timeout.count());
                    break;
                case event::Report::Type::Weird:
                    ESP_LOGI("Blinky", "I have been awake for %lums.", (xf::time::now() - start).count());
                    break;
                }
            }
        } else {
            m_led_state = !m_led_state;
            ESP_LOGI("Blinky", "Toggled led");
        }
    }
}
