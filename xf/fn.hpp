#pragma once

#include <type_traits>

namespace xf {

/// Allows a callback to dictate the control flow of the calling function's loop.
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

/// Restricts a template parameter to a callable that matches the given function signature, e.g: `void(int, float)`.
template<typename FN, typename Signature>
concept Fn = detail::invocable_matches_signature<FN>(static_cast<Signature*>(nullptr));

}
