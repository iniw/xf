#pragma once

#include <concepts>
#include <freertos/FreeRTOS.h>
#include <memory>
#include <utility>

namespace xf::util {
template<typename T>
T* allocate() {
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "Dynamic allocation is not supported");
    return static_cast<T*>(pvPortMalloc(sizeof(T)));
}

inline void deallocate(void* ptr) {
    static_assert(configSUPPORT_DYNAMIC_ALLOCATION, "Dynamic allocation is not supported");
    vPortFree(ptr);
}

template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
T* create(Args&&... args) {
    auto* storage = allocate<T>();
    if (storage == nullptr)
        return nullptr;
    return std::construct_at(storage, std::forward<Args>(args)...);
}

template<typename T>
void destroy(T* ptr) {
    configASSERT(ptr);
    ptr->~T();
    deallocate(ptr);
}
}
