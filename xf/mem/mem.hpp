#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include <freertos/FreeRTOS.h>

namespace xf::mem {

/// Allocate a block of memory for `T` on the FreeRTOS heap.
template<typename T>
T* allocate() {
    return static_cast<T*>(pvPortMalloc(sizeof(T)));
}

/// Deallocates a block of memory allocated on the FreeRTOS heap.
inline void deallocate(void* ptr) {
    vPortFree(ptr);
}

/// Allocate enough storage for `T` on the FreeRTOS heap and constructs an object in that location using the provided arguments.
/// Returns `nullptr` on allocation failure.
template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
T* create(Args&&... args) {
    auto* storage = allocate<T>();
    if (storage == nullptr)
        return nullptr;
    return std::construct_at(storage, std::forward<Args>(args)...);
}

/// Destroys a non-null, FreeRTOS-allocated object and then deallocates it's memory.
template<typename T>
void destroy(T* ptr) {
    configASSERT(ptr);
    ptr->~T();
    deallocate(ptr);
}

}
