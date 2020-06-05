#pragma once

namespace Iridium
{
    class StreamPosition
    {
    public:
        IR_FORCEINLINE constexpr StreamPosition() noexcept = default;

        IR_FORCEINLINE constexpr StreamPosition(u64 value) noexcept
            : value_(value)
        {}

        IR_FORCEINLINE constexpr i64 get() const noexcept
        {
            return static_cast<i64>(value_);
        }

        IR_FORCEINLINE constexpr u64 get(u64 default_value) const noexcept
        {
            return (value_ != UINT64_MAX) ? value_ : default_value;
        }

        IR_FORCEINLINE constexpr bool valid() const noexcept
        {
            return value_ != UINT64_MAX;
        }

        IR_FORCEINLINE static constexpr StreamPosition Checked(i64 value) noexcept
        {
            return (value >= 0) ? StreamPosition(static_cast<u64>(value)) : StreamPosition();
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
        u64 value_ {UINT64_MAX};
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
        return lhs.valid() ? StreamPosition::Checked(lhs.get() + rhs) : StreamPosition();
    }

    IR_FORCEINLINE constexpr StreamPosition& operator+=(StreamPosition& lhs, i64 rhs) noexcept
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
