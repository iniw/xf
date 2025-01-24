#pragma once

#include <cmath>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <xf/time/time.hpp>
#include <xf/util/fn.hpp>

namespace xf::task {
class Notifier {
public:
    void clear() {
        clear_state();
        clear_value();
    }

    void clear_state() {
        xTaskNotifyStateClearIndexed(handle, index);
    }

    void clear_value() {
        ulTaskNotifyValueClearIndexed(handle, index, UINT32_MAX);
    }

    const TaskHandle_t& handle;
    UBaseType_t index;
};

class CountingNotifier : public Notifier {
public:
    void give() {
        (void)xTaskNotifyGiveIndexed(handle, index);
    }

    [[nodiscard]] uint32_t await_take() {
        return take(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> take(std::chrono::duration<Rep, Period> timeout) {
        const auto value = ulTaskNotifyTakeIndexed(index, pdTRUE, time::to_raw_tick(timeout));
        if (value == pdFALSE)
            return std::nullopt;

        return value;
    }

    [[nodiscard]] uint32_t await_fetch() {
        return fetch(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> fetch(std::chrono::duration<Rep, Period> timeout) {
        uint32_t result = 0;
        const auto received = xTaskNotifyWaitIndexed(index, 0, 0, &result, time::to_raw_tick(timeout));
        if (received == pdFALSE)
            return std::nullopt;

        return result;
    }

    uint32_t current_value() const {
        return ulTaskNotifyValueClearIndexed(handle, index, 0);
    }

    uint32_t consume_value() const {
        return ulTaskNotifyValueClearIndexed(handle, index, UINT32_MAX);
    }

    void clear() {
        (void)ulTaskNotifyValueClearIndexed(handle, index, UINT32_MAX);
    }
};

class BinaryNotifier : public Notifier {
public:
    void set() {
        (void)xTaskNotifyIndexed(handle, index, true, eSetValueWithOverwrite);
    }

    void await_get() {
        auto ret = get(time::FOREVER);
        configASSERT(ret == true);
    }

    template<typename Rep, typename Period>
    [[nodiscard]] bool get(std::chrono::duration<Rep, Period> timeout) {
        return static_cast<bool>(xTaskNotifyWaitIndexed(index, 0, UINT32_MAX, nullptr, time::to_raw_tick(timeout)));
    }

    [[nodiscard]] bool current_value() {
        return ulTaskNotifyValueClearIndexed(handle, index, 0);
    }
};

template<typename T>
class StateNotifier : public Notifier {
public:
    static_assert(sizeof(T) <= sizeof(uint32_t) and std::is_trivially_copyable_v<T>,
        "Type can't be stored in a FreeRTOS notification");

    void set(T state) {
        uint32_t raw_value;
        std::memcpy(&raw_value, &state, sizeof(T));
        xTaskNotifyIndexed(handle, index, raw_value, eSetValueWithOverwrite);
    }

    [[nodiscard]] T await_get() {
        return get(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<T> get(std::chrono::duration<Rep, Period> timeout) {
        uint32_t raw_value = 0;
        const auto received = xTaskNotifyWaitIndexed(index, 0, UINT32_MAX, &raw_value, time::to_raw_tick(timeout));
        if (received == pdFALSE)
            return std::nullopt;

        alignas(T) std::byte buffer[sizeof(T)];
        std::memcpy(buffer, &raw_value, sizeof(T));
        return *std::launder(reinterpret_cast<T*>(&buffer));
    }
};

template<typename T, size_t NUM_STATES, size_t NUM_GROUPS>
class GroupStateNotifier : public Notifier {
public:
    static constexpr size_t BITS_PER_GROUP = std::ceil(std::log2(NUM_STATES));
    static_assert(BITS_PER_GROUP * NUM_GROUPS <= sizeof(uint32_t) * CHAR_BIT,
        "Not enough bits in the notification");

    // This function requires two critical sections, one for fetching the value and one for updating it, which is unfortunate.
    // Ideally FreeRTOS would provide a way to override specific bits in a task's notification, but it doesn't, so we have to do it manually.
    // `xTaskNotifyIndexed` with `eSetBits` may look promising, but it XOR's instead of overriding, which is not what we want.
    void set(uint8_t group, T state) {
        const auto raw_state = static_cast<uint32_t>(state);
        configASSERT(raw_state < NUM_STATES);

        uint32_t raw_value = ulTaskNotifyValueClearIndexed(handle, index, 0);
        raw_value = set_bits(raw_value, raw_state, group * BITS_PER_GROUP, BITS_PER_GROUP);
        xTaskNotifyIndexed(handle, index, raw_value, eSetValueWithOverwrite);
    }

    void set(std::array<T, NUM_GROUPS> states) {
        uint32_t raw_value = 0;
        for (size_t i = 0; i < NUM_GROUPS; ++i) {
            const auto raw_state = static_cast<uint32_t>(states[i]);
            configASSERT(raw_state < NUM_STATES);

            raw_value = set_bits(raw_value, raw_state, i * BITS_PER_GROUP, BITS_PER_GROUP);
        }
        xTaskNotifyIndexed(handle, index, raw_value, eSetValueWithOverwrite);
    }

    [[nodiscard]] std::array<T, NUM_GROUPS> await_get() {
        return get(time::FOREVER).value();
    }

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<std::array<T, NUM_GROUPS>> get(std::chrono::duration<Rep, Period> timeout) {
        uint32_t raw_value = 0;
        const auto received = xTaskNotifyWaitIndexed(index, 0, 0, &raw_value, time::to_raw_tick(timeout));
        if (received == pdFALSE)
            return std::nullopt;

        return raw_value_to_state_list(raw_value);
    }

private:
    // Credits to: https://stackoverflow.com/a/1283878
    static uint32_t set_bits(uint32_t destination, uint32_t source, uint8_t at, uint8_t num_bits) {
        const auto mask = (UINT32_MAX >> (sizeof(uint32_t) * CHAR_BIT - num_bits)) << at;
        return (destination & ~mask) | ((source << at) & mask);
    }

    static std::array<T, NUM_GROUPS> raw_value_to_state_list(uint32_t raw_value) {
        std::array<T, NUM_GROUPS> states {};
        for (size_t i = 0; i < NUM_GROUPS; ++i) {
            const auto group_state = (raw_value >> (i * BITS_PER_GROUP)) & ((1 << BITS_PER_GROUP) - 1);
            states[i] = static_cast<T>(group_state);
        }
        return states;
    }
};

template<typename T>
class Task {
public:
    using Handle = TaskHandle_t;

    Task() = default;

    Task(Task&& other) noexcept
        : m_handle(std::exchange(other.m_handle, nullptr)) {
    }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (m_handle)
                destroy();
            m_handle = std::exchange(other.m_handle, nullptr);
        }
        return *this;
    }

    ~Task() {
        if (m_handle)
            destroy();
    }

    // There are no mechanism in FreeRTOS to copy a task
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

#ifdef ESP_PLATFORM
    [[nodiscard]] bool create(const char* name, uint32_t stack_size, UBaseType_t priority, BaseType_t core_id = -1) {
        configASSERT(m_handle == nullptr);
        if (core_id == -1) {
            return xTaskCreate(task, name, stack_size, this, priority, &m_handle) == pdPASS;
        } else {
            return xTaskCreatePinnedToCore(task, name, stack_size, this, priority, &m_handle, core_id) == pdPASS;
        }
    }

    [[nodiscard]] bool create(uint32_t stack_size, UBaseType_t priority, BaseType_t core_id = -1) {
        return create(nullptr, stack_size, priority, core_id);
    }
#else
    [[nodiscard]] bool create(const char* name, uint32_t stack_size, UBaseType_t priority) {
        configASSERT(m_handle == nullptr);
        return xTaskCreate(task, name, stack_size, this, priority, &m_handle) == pdPASS;
    }

    [[nodiscard]] bool create(uint32_t stack_size, UBaseType_t priority) {
        return create(nullptr, stack_size, priority);
    }
#endif

    void suspend() {
        vTaskSuspend(m_handle);
    }

    void resume() {
        vTaskResume(m_handle);
    }

    [[nodiscard]] Handle raw_handle() const {
        return m_handle;
    }

    [[nodiscard]] UBaseType_t priority() const {
        return uxTaskPriorityGet(m_handle);
    }

    void set_priority(UBaseType_t p) {
        return vTaskPrioritySet(m_handle, p);
    }

    [[nodiscard]] bool is_running() const {
        return m_handle != nullptr;
    }

protected:
    template<typename Rep, typename Period>
    void suspend_for(std::chrono::duration<Rep, Period> duration) {
        vTaskDelay(time::to_raw_tick(duration));
    }

    template<typename Rep, typename Period>
    time::Tick suspend_until(time::Tick previous_wake_time, std::chrono::duration<Rep, Period> increment) {
        auto raw = previous_wake_time.time_since_epoch().count();
        vTaskDelayUntil(&raw, time::to_raw_tick(increment));
        return time::Tick { time::Duration { raw } };
    }

    template<typename Rep, typename Period>
    void every(std::chrono::duration<Rep, Period> period, util::ControlFlowFn auto&& fn) {
        auto time = time::now();
        while (true) {
            time = suspend_until(time, period);
            if (std::invoke(fn) == util::ControlFlow::Break)
                break;
        }
    }

    static void task(void* raw_self) {
        auto& self = *static_cast<Task*>(raw_self);

        if constexpr (HAS_SETUP) {
            if constexpr (HAS_FALLIBLE_SETUP) {
                if (not self.setup()) {
                    self.destroy();
                    return;
                }
            } else {
                self.setup();
            }
        }

        self.run();

        self.destroy();
    }

    void destroy() {
        configASSERT(m_handle);
        vTaskDelete(std::exchange(m_handle, nullptr));
    }

    [[nodiscard]] UBaseType_t stack_high_mark() const {
        return uxTaskGetStackHighWaterMark(m_handle);
    }

private:
    static constexpr bool HAS_SETUP = requires { std::declval<T>().setup_impl(); };
    static constexpr bool HAS_FALLIBLE_SETUP = requires { { std::declval<T>().setup_impl() } -> std::convertible_to<bool>; };

    // Implementing this is optional
    void setup()
    requires(not HAS_FALLIBLE_SETUP)
    {
        static_cast<T*>(this)->setup_impl();
    }

    // Implementing this is optional
    [[nodiscard]] decltype(auto) setup()
    requires(HAS_FALLIBLE_SETUP)
    {
        return static_cast<T*>(this)->setup_impl();
    }

    void run() {
        static_cast<T*>(this)->run_impl();
    }

protected:
    Handle m_handle { nullptr };

    template<std::derived_from<Notifier>... N>
    friend class Notifiable;
};

#define XF_TASK        \
private:               \
    template<typename> \
    friend class ::xf::task::Task

template<std::derived_from<Notifier>... N>
class Notifiable {
    static_assert(sizeof...(N) > 0, "At least one notifier must be provided");
    static_assert(sizeof...(N) <= configTASK_NOTIFICATION_ARRAY_ENTRIES);

protected:
    template<size_t I = tskDEFAULT_INDEX_TO_NOTIFY>
    [[nodiscard]] auto& notification() {
        return std::get<I>(m_task_notifications);
    }

    template<typename Notifier>
    [[nodiscard]] auto& notification() {
        return std::get<Notifier>(m_task_notifications);
    }

    // Incrementing `index` sequentially here is not UB because `Notifier` is a trivially-copyable type, making `m_notifiers` also trivally-copyable,
    // So this is direct-list-initialization, meaning the parameters are formally evaluated in left-to-right order.
    explicit Notifiable(auto& task, size_t index = 0)
        : m_task_notifications {
            N { task.m_handle, index++ }...
        } {
    }

private:
    std::tuple<N...> m_task_notifications;
};

template<typename T, size_t STACK_SIZE>
class StaticTask : public Task<T> {
public:
    static_assert(STACK_SIZE >= configMINIMAL_STACK_SIZE);

#ifdef ESP_PLATFORM
    void create(const char* name, UBaseType_t priority, BaseType_t core_id = -1) {
        configASSERT(this->m_handle == nullptr);
        if (core_id == -1) {
            this->m_handle = xTaskCreateStatic(Task<T>::task, name, STACK_SIZE, this, priority, m_stack_buffer.data(), &m_static_task);
        } else {
            this->m_handle = xTaskCreateStaticPinnedToCore(Task<T>::task, name, STACK_SIZE, this, priority, m_stack_buffer.data(), &m_static_task, core_id);
        }
    }

    void create(UBaseType_t priority, BaseType_t core_id = -1) {
        create(nullptr, priority, core_id);
    }
#else
    void create(const char* name, UBaseType_t priority) {
        configASSERT(this->m_handle == nullptr);
        this->m_handle = xTaskCreateStatic(Task<T>::task, name, STACK_SIZE, this, priority, m_stack_buffer.data(), &m_static_task);
    }

    void create(UBaseType_t priority) {
        create(nullptr, priority);
    }
#endif

private:
    // Hide the visibility of the `create` function from the base class, since that has the stack size parameter
    using Task<T>::create;

    StaticTask_t m_static_task;
    std::array<StackType_t, STACK_SIZE> m_stack_buffer;
};
}
