#pragma once

#include <type_traits>

namespace xf::util {
enum class ControlFlow {
    Continue,
    Break
};

namespace detail {
template<typename FN, typename Ret, typename... Args>
consteval bool invocable_matches_signature(Ret (*)(Args...)) {
    if constexpr (std::is_invocable_v<FN, Args...>) {
        return std::is_same_v<std::invoke_result_t<FN, Args...>, Ret>;
    } else {
        return false;
    }
}
}

template<typename FN, typename Signature>
concept Fn = detail::invocable_matches_signature<FN>(static_cast<Signature*>(nullptr));

template<typename FN, typename... Args>
concept ControlFlowFn = Fn<FN, ControlFlow(Args...)>;
}
