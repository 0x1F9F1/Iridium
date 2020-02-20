#pragma once

#include <cstddef>
#include <cstdint>

namespace Iridium
{
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8fast = std::int_fast8_t;
    using i16fast = std::int_fast16_t;
    using i32fast = std::int_fast32_t;
    using i64fast = std::int_fast64_t;

    using u8fast = std::uint_fast8_t;
    using u16fast = std::uint_fast16_t;
    using u32fast = std::uint_fast32_t;
    using u64fast = std::uint_fast64_t;

    using f32 = float;
    using f64 = double;

    using usize = std::size_t;
    using isize = std::ptrdiff_t;

    inline namespace Literals
    {
#define _IR_LITERAL(NAME, IN_TYPE, OUT_TYPE)                              \
    IR_FORCEINLINE constexpr OUT_TYPE operator"" NAME(IN_TYPE n) noexcept \
    {                                                                     \
        return static_cast<OUT_TYPE>(n);                                  \
    }

        _IR_LITERAL(_i8, unsigned long long int, i8);
        _IR_LITERAL(_i16, unsigned long long int, i16);
        _IR_LITERAL(_i32, unsigned long long int, i32);
        _IR_LITERAL(_i64, unsigned long long int, i64);

        _IR_LITERAL(_u8, unsigned long long int, u8);
        _IR_LITERAL(_u16, unsigned long long int, u16);
        _IR_LITERAL(_u32, unsigned long long int, u32);
        _IR_LITERAL(_u64, unsigned long long int, u64);

        _IR_LITERAL(_f32, long double, f32);
        _IR_LITERAL(_f64, long double, f64);

        _IR_LITERAL(_isize, unsigned long long int, isize);
        _IR_LITERAL(_usize, unsigned long long int, usize);

#undef _IR_LITERAL
    } // namespace Literals
} // namespace Iridium
