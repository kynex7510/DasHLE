#ifndef _DASHLE_SUPPORT_MATH_H
#define _DASHLE_SUPPORT_MATH_H

#include "DasHLE/Support/Types.h"

#include <concepts>

namespace dashle {

template <std::integral T>
constexpr bool isPowerOfTwo(T val) {
    return val && !(val & (val - 1));
}

template <std::integral T>
constexpr Expected<T> alignDown(T val, T alignment) {
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

#endif /* _DASHLE_SUPPORT_MATH_H */