#pragma once

#include "Queue.hpp"

namespace xf::queue {

/// A statically allocated version of `Queue`.
/// The queue's length is set and underlying memory is allocated based on the `LENGTH` template parameter.
/// Refer to `Queue`'s documentation for more information.
template<typename Item, size_t LENGTH>
class StaticQueue : public Queue<Item> {
public:
    static_assert(LENGTH > 0, "Static queue size must be at least 1");

    /// Creates the queue using the length from the class template parameter, which is the maximum number of items the queue can hold.
    /// Analogous to [`xQueueCreateStatic`](https://www.freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/02-xQueueCreateStatic).
    void create() {
        configASSERT(this->m_handle == nullptr);
        this->m_handle = xQueueCreateStatic(LENGTH, sizeof(typename Queue<Item>::StoredItem), m_static_storage.data(), &m_static_queue);
    }

private:
    // Hide the visibility of the `create` function from the base class since that has a length parameter, which we take as a template argument.
    // The replacement, which doesn't take the length parameter, is declared above.
    using Queue<Item>::create;

    StaticQueue_t m_static_queue;
    std::array<std::uint8_t, LENGTH * sizeof(typename Queue<Item>::StoredItem)> m_static_storage;
};

}
