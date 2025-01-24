#pragma once

#include <cstring>
#include <xf/Queue.hpp>
#include <xf/isr/time/time.hpp>
#include <xf/util/fn.hpp>

namespace xf::isr {
using HigherPriorityTaskWoken = bool;

void yield(std::convertible_to<HigherPriorityTaskWoken> auto... yield) {
    if (true and (HigherPriorityTaskWoken(yield) or ...))
        portYIELD_FROM_ISR();
}

template<typename T>
class ISR {
protected:
    ISR(const ISR&) = delete;
    ISR& operator=(const ISR&) = delete;
    ISR(ISR&&) = delete;
    ISR& operator=(ISR&&) = delete;

public:
    void start() {
        configASSERT(s_instance == nullptr);

        s_instance = this;

        if constexpr (requires { std::declval<T>().setup_impl(); }) {
            static_cast<T*>(this)->setup_impl();
        }
    }

    void stop() {
        configASSERT(s_instance);
        s_instance = nullptr;
    }

    // Unique for each specialization of this class
    static void IRAM_ATTR isr() {
        yield(static_cast<T*>(s_instance)->isr_impl());
    }

    ~ISR() {
        if (s_instance)
            stop();
    }

private:
    static inline ISR* s_instance { nullptr };
};

#define XF_ISR                 \
private:                       \
    template<typename, size_t> \
    friend class ::xf::isr::ISR

template<typename T, TickType_t MS>
class DebouncedISR : public ISR<T> {
    XF_ISR;

protected:
    using ISR<T>::ISR;

private:
    xf::isr::HigherPriorityTaskWoken isr_impl() {
        const auto now = time::now();
        if (now - std::exchange(m_last_trigger, now) >= DEBOUNCE_TIMER)
            return static_cast<T*>(this)->debounced_isr_impl();

        return false;
    }

    static constexpr time::Duration DEBOUNCE_TIMER { MS };

    time::Tick m_last_trigger;
};

#define XF_DEBOUNCED_ISR                   \
private:                                   \
    template<typename, TickType_t, size_t> \
    friend class ::xf::isr::DebouncedISR
}
