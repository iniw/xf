#include <esp_log.h>
#include <xf/task/StaticTask.hpp>
#include <xf/timer/Timer.hpp>

class Task : public xf::task::StaticTask<2048> {
    void setup() override;

    void run() override;

public:
    Task()
        : m_timer(
              xf::timer::Mode::Repeating,
              [](int& counter) {
                  counter = 0;
                  ESP_LOGI("Timer", "Reset the counter back to 0");
              },
              m_counter) { }

private:
    xf::timer::Timer<int&> m_timer;

    int m_counter { 0 };
};
void Task::setup() {
    m_timer.create("Example timer", std::chrono::seconds { 25 });
}

void Task::run() {
    every(std::chrono::seconds { 5 }, [&] {
        m_counter += 100;
        ESP_LOGI("Task", "Increased counter (value = %d)", m_counter);
    });
}

extern "C" void app_main(void) {
    static Task task;
    task.create("Task", 10);
}
