#include <esp_log.h>
#include <xf/queue/StaticQueue.hpp>
#include <xf/task/StaticTask.hpp>

// TaskA and TaskB are tightly coupled via circular queue communication.
// The queues are not owned by the tasks themselves to avoid a chicken-and-egg problem during construction.
// If they owned their queues, constructing either task would require the other to already exist.
class TaskA : public xf::task::StaticTask<4096> {
    void run() override;

public:
    using Queue = xf::queue::StaticQueue<float, 5>;

    TaskA(Queue& queue, xf::queue::StaticQueue<int, 5>& task_b_queue)
        : m_queue(queue)
        , m_task_b_queue(task_b_queue) { }

private:
    Queue& m_queue;                                 // Receives float events from TaskB
    xf::queue::StaticQueue<int, 5>& m_task_b_queue; // Sends int events to TaskB
};

void TaskA::run() {
    while (true) {
        auto _ = m_queue.await_receive(); // Wait for float from TaskB
        m_task_b_queue.await_send(47);    // Respond with int to TaskB
    }
}

class TaskB : public xf::task::StaticTask<4096> {
    void run() override;

public:
    using Queue = xf::queue::StaticQueue<int, 5>;

    TaskB(Queue& queue, TaskA::Queue& task_a_queue)
        : m_queue(queue)
        , m_task_a_queue(task_a_queue) { }

private:
    Queue& m_queue;               // Receives int from TaskA
    TaskA::Queue& m_task_a_queue; // Sends float to TaskA
};

void TaskB::run() {
    while (true) {
        auto _ = m_queue.await_receive(); // Wait for int from TaskA
        m_task_a_queue.await_send(55.0f); // Respond with float to TaskA
    }
}

// The Maestro orchestrates task setup, event dispatching, and provides a central point for I/O integration.
// It owns all tasks and queues and because everything is statically allocated the entire firmware (or at least the part that we as users are writing) resides in a large contiguous block in RAM.
// Centralized setup also enables testing and mocking external events.
class Maestro : public xf::task::StaticTask<4096> {
    void setup() override;

    void run() override;

public:
    using Queue = xf::queue::StaticQueue<std::variant<float, int>, 5>;

    Maestro();

private:
    Queue m_queue; // Ingress point for external events (e.g., deserialized input)

    TaskA::Queue m_task_a_queue;
    TaskB::Queue m_task_b_queue;

    TaskA m_task_a;
    TaskB m_task_b;
};

Maestro::Maestro()
    : m_task_a(m_task_a_queue, m_task_b_queue)
    , m_task_b(m_task_b_queue, m_task_a_queue) {
    // Bootstrap the maestro and make it the highest priority task.
    create("Maestro", 10);
}

void Maestro::setup() {
    m_queue.create();

    m_task_a_queue.create();
    m_task_b_queue.create();

    m_task_a.create(5); // Priority 5
    m_task_b.create(5); // Priority 5
}

void Maestro::run() {
    while (true) {
        // Maestro decodes events and routes them to appropriate handlers.
        auto variant = m_queue.await_receive();
        if (auto* event = std::get_if<float>(&variant)) {
            m_task_a_queue.await_send(*event); // Forward to TaskA
        } else if (auto* event = std::get_if<int>(&variant)) {
            m_task_b_queue.await_send(*event); // Forward to TaskB
        }
    }
}

extern "C" void app_main(void) {
    static Maestro maestro;
}
