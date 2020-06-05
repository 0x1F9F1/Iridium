#pragma once

// May perform better on certain architectures without cmov
// #define IR_STREAM_POS_USE_SAR

namespace Iridium
{
    // An i64 wrapper designed to fast, easy handling of invalid (negative) values
    // The checks should only add 1 to 3 instructions each, depending on usage
    class StreamPosition
    {
    public:
        IR_FORCEINLINE constexpr StreamPosition() noexcept = default;

        IR_FORCEINLINE constexpr StreamPosition(i64 value) noexcept
#ifdef IR_STREAM_POS_USE_SAR
            : value_(value | (value >> 63)) // mov, sar, or
#else
            : value_(value < 0 ? -1 : value) // mov reg -1, cmp/test/add, cmovs
#endif
        {}

        IR_FORCEINLINE constexpr i64 get() const noexcept
        {
            return value_;
        }

        IR_FORCEINLINE constexpr u64 get(u64 default_value) const noexcept
        {
            return (value_ >= 0) ? value_ : default_value;
        }

        IR_FORCEINLINE constexpr bool valid() const noexcept
        {
            return value_ >= 0;
        }

        friend constexpr bool operator==(StreamPosition lhs, StreamPosition rhs) noexcept;
        friend constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs) noexcept;
        friend constexpr bool operator<(StreamPosition lhs, StreamPosition rhs) noexcept;
        friend constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs) noexcept;
        friend constexpr bool operator>(StreamPosition lhs, StreamPosition rhs) noexcept;
        friend constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs) noexcept;

        friend constexpr StreamPosition operator+(StreamPosition lhs, i64 rhs) noexcept;
        friend constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs) noexcept;

    private:
        i64 value_ {-1};
    };

    IR_FORCEINLINE constexpr bool operator==(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ != rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator<(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ < rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ <= rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator>(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ > rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs) noexcept
    {
        return lhs.value_ >= rhs.value_;
    }

    IR_FORCEINLINE constexpr StreamPosition operator+(StreamPosition lhs, i64 rhs) noexcept
    {
        StreamPosition result = lhs;

#ifdef IR_STREAM_POS_USE_SAR
        result.value_ = (result.value_ + rhs) | (result.value_ >> 63);
#else
        result.value_ += (result.value_ < 0) ? 0 : rhs;
#endif

        return result;
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs) noexcept
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
