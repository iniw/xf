#include <esp_log.h>
#include <xf/task/StaticTask.hpp>

// `Blinky` is an object representing a statically allocated task with 4096 "bytes" of stack depth.
class Blinky : public xf::task::StaticTask<4096> {
    // The optional setup function, will run once after the task is created,
    void setup() override;

    // The core task function, will run once after the task is created and `setup()` has returned.
    // When this function returns the task will be destroyed automatically.
    void run() override;

public:
    Blinky() = default;

private:
    bool m_state { false };
};

void Blinky::setup() {
    ESP_LOGI("Blinky", "Hello wordl!");
}

void Blinky::run() {
    while (true) {
        ESP_LOGI("Blinky", "Turning LED %s", m_state ? "Off" : "On");

        // Homework: use `m_state` for something.
        m_state = !m_state;

        // Sleep for a bit
        delay(std::chrono::seconds { 1 });
    }
}

extern "C" void app_main(void) {
    // `app_main` is itself a task will return and destroy everything in it's stack, so mark our object as `static` to make it immortal.
    static Blinky blinky;

    constexpr auto NAME = "Blinky";
    constexpr auto PRIORITY = 5;

    blinky.create(NAME, PRIORITY);
}
