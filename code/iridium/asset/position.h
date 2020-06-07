#pragma once

namespace Iridium
{
    class StreamPosition
    {
    public:
        IR_FORCEINLINE constexpr StreamPosition() noexcept = default;

        IR_FORCEINLINE constexpr StreamPosition(u64 value) noexcept
            : value(static_cast<i64>(value))
        {}

        IR_FORCEINLINE constexpr StreamPosition(i64 value) noexcept
            : value(value < 0 ? -1 : value) // mov reg -1, cmp/test/add, cmovs
        {}

        IR_FORCEINLINE constexpr i64 get() const noexcept
        {
            return value;
        }

        IR_FORCEINLINE constexpr u64 get(u64 default_value) const noexcept
        {
            return (value >= 0) ? value : default_value;
        }

        IR_FORCEINLINE constexpr bool valid() const noexcept
        {
            return value >= 0;
        }

        i64 value {-1};
    };

    IR_FORCEINLINE constexpr bool operator==(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value == rhs.value;
    }

    IR_FORCEINLINE constexpr bool operator!=(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value != rhs.value;
    }

    IR_FORCEINLINE constexpr bool operator<(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value < rhs.value;
    }

    IR_FORCEINLINE constexpr bool operator<=(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value <= rhs.value;
    }

    IR_FORCEINLINE constexpr bool operator>(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value > rhs.value;
    }

    IR_FORCEINLINE constexpr bool operator>=(StreamPosition const lhs, StreamPosition const rhs) noexcept
    {
        return lhs.value >= rhs.value;
    }

    IR_FORCEINLINE constexpr StreamPosition operator+(StreamPosition const lhs, i64 rhs) noexcept
    {
        rhs += lhs.value;

        StreamPosition result;
        result.value = ((rhs | lhs.value) < 0) ? -1 : rhs;
        return result;
    }

    IR_FORCEINLINE constexpr StreamPosition operator+(StreamPosition const lhs, u64 rhs) noexcept
    {
        StreamPosition result = lhs;
        result.value += (result.value < 0) ? 0 : rhs;
        return result;
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs) noexcept
    {
        lhs = lhs + rhs;

        return lhs;
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, u64 rhs) noexcept
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
