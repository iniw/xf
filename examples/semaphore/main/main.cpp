#include <esp_log.h>
#include <xf/semaphore/MutexProtected.hpp>
#include <xf/task/StaticTask.hpp>

// A statically allocated mutex protected integer.
using ProtectedData = xf::semaphore::MutexProtected<int>;

// Our first task. Will fight for access over the shared data against `B`.
class TaskA : public xf::task::StaticTask<4096> {
    void run() override;

public:
    TaskA(ProtectedData& data)
        : m_protected_data(data) { }

private:
    ProtectedData& m_protected_data;
};

void TaskA::run() {
    every(std::chrono::seconds { 10 }, [&] {
        m_protected_data.await_access([&](int& data) {
            int old = std::exchange(data, 55);
            ESP_LOGI("Task A", "Got data (old=%d, new=%d)", old, data);
        });
    });
}

// Our second task. Will fight for access over the shared data against `A`.
class TaskB : public xf::task::StaticTask<4096> {
    void run() override;

public:
    TaskB(ProtectedData& data)
        : m_protected_data(data) { }

private:
    ProtectedData& m_protected_data;
};

void TaskB::run() {
    every(std::chrono::seconds { 10 }, [&] {
        m_protected_data.await_access([&](int& data) {
            int old = std::exchange(data, 47);
            ESP_LOGI("Task B", "Got data (old=%d, new=%d)", old, data);
        });
    });
}

extern "C" void app_main(void) {
    static ProtectedData data { 0 };
    static TaskA task_a { data };
    static TaskB task_b { data };

    data.create();

    task_a.create("Task A", 10);
    task_b.create("Task B", 10);
}
