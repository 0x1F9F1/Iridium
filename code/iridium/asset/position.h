#pragma once

namespace Iridium
{
    class StreamPosition
    {
    public:
        inline constexpr StreamPosition() = default;

        inline constexpr StreamPosition(u64 value)
            : value_(value)
        {}

        inline constexpr i64 get() const
        {
            return static_cast<i64>(value_);
        }

        inline constexpr u64 get(u64 default_value) const
        {
            return (value_ != UINT64_MAX) ? value_ : default_value;
        }

        inline constexpr bool valid() const
        {
            return value_ != UINT64_MAX;
        }

        friend constexpr bool operator==(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator<(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator>(StreamPosition lhs, StreamPosition rhs);
        friend constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs);

        friend constexpr StreamPosition operator+(StreamPosition lhs, StreamPosition rhs);
        friend constexpr StreamPosition& operator+=(StreamPosition& lhs, StreamPosition rhs);

    private:
        u64 value_ {UINT64_MAX};
    };

    inline constexpr bool operator==(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ == rhs.value_;
    }

    inline constexpr bool operator!=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ != rhs.value_;
    }

    inline constexpr bool operator<(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ < rhs.value_;
    }

    inline constexpr bool operator<=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ <= rhs.value_;
    }

    inline constexpr bool operator>(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ > rhs.value_;
    }

    inline constexpr bool operator>=(StreamPosition lhs, StreamPosition rhs)
    {
        return lhs.value_ >= rhs.value_;
    }

    inline constexpr StreamPosition operator+(StreamPosition lhs, StreamPosition rhs)
    {
        if (lhs.valid() && rhs.valid())
            return StreamPosition(lhs.value_ + rhs.value_);

        return StreamPosition();
    }

    inline constexpr StreamPosition& operator+=(StreamPosition& lhs, StreamPosition rhs)
    {
        lhs = lhs + rhs;

        return lhs;
    }
} // namespace Iridium
