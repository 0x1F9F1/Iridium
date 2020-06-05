#pragma once

namespace Iridium
{
    class StreamPosition
    {
    public:
        IR_FORCEINLINE constexpr StreamPosition() noexcept = default;

        IR_FORCEINLINE constexpr StreamPosition(u64 value) noexcept
            : value_(static_cast<i64>(value))
        {}

        IR_FORCEINLINE constexpr StreamPosition(i64 value) noexcept
            : value_(value < 0 ? -1 : value) // mov reg -1, cmp/test/add, cmovs
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
        rhs += lhs.value_;

        StreamPosition result;
        result.value_ = ((rhs | lhs.value_) < 0) ? -1 : rhs;
        return result;
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs) noexcept
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
