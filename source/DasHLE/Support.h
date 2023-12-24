#ifndef _DASHLE_SUPPORT_H
#define _DASHLE_SUPPORT_H

#include "DasHLE/Base.h"

#include <string>
#include <format>
#include <type_traits>
#include <concepts>

#if defined(NDEBUG)

#define DASHLE_LOG_LINE(...)
#define DASHLE_ASSERT(cond) (cond)
#define DASHLE_UNREACHABLE(...) ::std::abort()

#else

#define DASHLE_LOG_LINE(...) ::dashle::_internal::logLine(std::format(__VA_ARGS__))

#define DASHLE_ASSERT(cond)                                 \
    do {                                                    \
        if (!(cond)) {                                      \
            DASHLE_LOG_LINE("Assertion failed: {}", #cond); \
            DASHLE_LOG_LINE("In file: {}", __FILE__);       \
            DASHLE_LOG_LINE("On line: {}", __LINE__);       \
            ::std::abort();                                 \
        }                                                   \
    } while (false)

#define DASHLE_UNREACHABLE(...)       \
    do {                              \
        DASHLE_LOG_LINE(__VA_ARGS__); \
        ::std::abort();               \
    } while (false)

#endif // NDEBUG

#define DASHLE_TRY_EXPECTED(name, expr)                 \
    auto _##name##Wrapper_ = (expr);                    \
    if (!(_##name##Wrapper_))                           \
        return Unexpected((_##name##Wrapper_).error()); \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_EXPECTED_CONST(name, expr)              \
    const auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                              \
        return Unexpected((_##name##Wrapper_).error());    \
    const auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_EXPECTED_VOID(expr)                \
    {                                                 \
        const auto _voidWrapper_ = (expr);            \
        if (!_voidWrapper_)                           \
            return Unexpected(_voidWrapper_.error()); \
    }

#define DASHLE_TRY_OPTIONAL(name, expr, error)       \
    auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                        \
        return Unexpected((error));                  \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_TRY_OPTIONAL_CONST(name, expr, error)       \
    const auto _##name##Wrapper_ = (expr);                 \
    if (!(_##name##Wrapper_))                              \
        return Unexpected((error));                        \
    const auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_ASSERT_WRAPPER(name, expr)            \
    auto _##name##Wrapper_ = (expr);                 \
    DASHLE_ASSERT(_##name##Wrapper_);                \
    auto name = std::move(_##name##Wrapper_.value())

#define DASHLE_ASSERT_WRAPPER_CONST(name, expr)            \
    const auto _##name##Wrapper_ = (expr);                 \
    DASHLE_ASSERT(_##name##Wrapper_);                      \
    const auto name = std::move(_##name##Wrapper_.value())

namespace dashle {

template <typename T, typename ... Args>
concept OneOf = (std::same_as<T, Args> || ...);

template <typename T>
concept Pointerable = std::is_pointer_v<T>;

template <typename Self, typename T>
struct DeducedConstImpl {
    using Type = std::conditional_t<std::is_const_v<Self>, std::add_const_t<T>, std::remove_const_t<T>>;
};

template <typename Self, Pointerable T>
struct DeducedConstImpl<Self, T> {
    using Type = std::conditional_t<std::is_const_v<Self>,
        std::add_pointer_t<std::add_const_t<std::remove_pointer_t<T>>>,
        std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<T>>>>;
};

template <typename Self, typename T>
using DeducedConst = typename DeducedConstImpl<Self, T>::Type;

template <typename ... Args>
struct AlignedStorage final {
    alignas(Args...) u8 buffer[std::max({sizeof(Args)...})];
};

template <typename Base, typename ... Derived>
requires (std::derived_from<Derived, Base> && ...)
class PolymorphicView final {
    AlignedStorage<Derived...> m_Storage;
    const Base* m_Ptr = nullptr;

    void copyFrom(const PolymorphicView& rhs) {
        if (rhs.m_Ptr) {
            std::copy(rhs.m_Storage.buffer, rhs.m_Storage.buffer + sizeof(m_Storage.buffer), m_Storage.buffer);
            m_Ptr = reinterpret_cast<const Base*>(m_Storage.buffer);
        } else {
            m_Ptr = nullptr;
        }
    }

public:
    PolymorphicView() {}
    PolymorphicView(const PolymorphicView& rhs) { copyFrom(rhs); }

    template <typename T, typename ... Args>
    requires(OneOf<T, Derived...>)
    void initialize(Args&&... args) {
        new (m_Storage.buffer) T(std::forward<Args>(args)...);
        m_Ptr = reinterpret_cast<const Base*>(m_Storage.buffer);
    }

    const Base* get() const { return m_Ptr; }

    const Base* operator->() const {
        DASHLE_ASSERT(m_Ptr);
        return m_Ptr;
    }
};

namespace _internal {
void logLine(const std::string& s);
} // namespace dashle::_internal

template <std::integral T>
constexpr bool isPowerOfTwo(T val) {
    return val && !(val & (val - 1));
}

template <std::integral T>
constexpr Expected<T> align(T val, T alignment) {
    if (dashle::isPowerOfTwo(alignment))
        return val & ~(alignment - 1);

    return Unexpected(Error::InvalidAlignment);
}

template <std::integral T>
constexpr Expected<T> alignOver(T val, T alignment) {
    if (dashle::isPowerOfTwo(alignment))
        return (val + (alignment - 1)) & ~(alignment - 1);

    return Unexpected(Error::InvalidAlignment);
}

} // namespace dashle

template<> 
struct std::formatter<dashle::Error> {
  constexpr auto parse(std::format_parse_context& ctx) { 
    return ctx.begin(); 
  }

  auto format(dashle::Error error, std::format_context& ctx) const { 
    return std::format_to(ctx.out(), "{}", dashle::errorAsString(error)); 
  } 
};

#endif /* _DASHLE_SUPPORT_H */