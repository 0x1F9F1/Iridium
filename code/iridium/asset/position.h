#pragma once

namespace Iridium
{
    class StreamPosition
    {
    public:
        IR_FORCEINLINE constexpr StreamPosition() = default;

        IR_FORCEINLINE constexpr StreamPosition(u64 value)
            : value_(value)
        {}

        IR_FORCEINLINE constexpr i64 get() const
        {
            return static_cast<i64>(value_);
        }

        IR_FORCEINLINE constexpr u64 get(u64 default_value) const
        {
            return (value_ != UINT64_MAX) ? value_ : default_value;
        }

        IR_FORCEINLINE constexpr bool valid() const
        {
            return value_ != UINT64_MAX;
        }

        IR_FORCEINLINE static constexpr StreamPosition Checked(i64 value)
        {
            return (value >= 0) ? StreamPosition(static_cast<u64>(value)) : StreamPosition();
        }

        friend constexpr bool operator==(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator<(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator>(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs);

        friend constexpr StreamPosition operator+(StreamPosition lhs, i64 rhs);
        friend constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs);

    private:
        u64 value_ {UINT64_MAX};
    };

    IR_FORCEINLINE constexpr bool operator==(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ == rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ != rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator<(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ < rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ <= rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator>(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ > rhs.value_;
    }

    IR_FORCEINLINE constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ >= rhs.value_;
    }

    IR_FORCEINLINE constexpr StreamPosition operator+(StreamPosition lhs, i64 rhs)
    {
        return lhs.valid() ? StreamPosition::Checked(lhs.get() + rhs) : StreamPosition();
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs)
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
