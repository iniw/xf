#include <esp_log.h>
#include <xf/queue/StaticQueue.hpp>
#include <xf/task/StaticTask.hpp>

// A statically allocated queue that stores a `std::string` and has a maximum length of 5.
// Notice how the object being stored (`std::string`) is not trivially copyable, meaning just a raw `memcpy()` into the queue's internal buffer, which is what FreeRTOS does, would easily cause memory safety issues.
// Yet this works! Dig into the documentation for `Queue` to understand how.
using Queue = xf::queue::StaticQueue<std::string, 5>;

// The producer of our queue.
// Gets access to it by way of dependency injection, which I've found to be the sanest way to do it.
class Producer : public xf::task::StaticTask<2048> {
    void run() override;

public:
    Producer(Queue& queue)
        : m_queue(queue) { }

private:
    Queue& m_queue;

    int m_counter { 0 };
};

void Producer::run() {
    every(std::chrono::seconds { 10 }, [&] {
        m_queue.await_send(std::to_string(m_counter++));
        ESP_LOGI("Task A", "Sent over an item");
    });
}

// The consumer of our queue.
// Gets access to it by way of dependency injection, which I've found to be the sanest way to do it.
class Consumer : public xf::task::StaticTask<2048> {
    void run() override;

public:
    Consumer(Queue& queue)
        : m_queue(queue) { }

private:
    Queue& m_queue;
};

void Consumer::run() {
    while (true) {
        auto item = m_queue.await_receive();
        ESP_LOGI("Task A", "Received an item (value=%s)", item.c_str());
    }
}

extern "C" void app_main(void) {
    static Queue queue;
    static Producer producer { queue };
    static Consumer consumer { queue };

    queue.create();

    producer.create(2);
    consumer.create(2);
}
