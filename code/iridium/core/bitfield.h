#pragma once

namespace Iridium
{
    template <usize Bits>
    struct smallest_int_for_bits_
    {
        using type = void;
    };

    template <>
    struct smallest_int_for_bits_<8>
    {
        using type = u8;
    };

    template <>
    struct smallest_int_for_bits_<16>
    {
        using type = u16;
    };

    template <>
    struct smallest_int_for_bits_<32>
    {
        using type = u32;
    };

    template <>
    struct smallest_int_for_bits_<64>
    {
        using type = u64;
    };

    template <usize Bits>
    struct smallest_int_for_bits
        : smallest_int_for_bits_<
              (Bits == 0) ? 0 : (Bits <= 8) ? 8 : (Bits <= 16) ? 16 : (Bits <= 32) ? 32 : (Bits <= 64) ? 64 : 0>
    {};

    template <typename T, usize Index, usize Bits>
    class bit_field
    {
    public:
        static constexpr T Mask = (T(1) << Bits) - 1;
        static constexpr T ShiftedMask = Mask << Index;

        using Type = typename smallest_int_for_bits<Bits>::type;

        IR_FORCEINLINE bit_field& operator=(Type value)
        {
            value_ ^= (value_ ^ (static_cast<T>(value) << Index)) & ShiftedMask;

            return *this;
        }

        IR_FORCEINLINE operator Type() const
        {
            return static_cast<Type>((value_ >> Index) & Mask);
        }

        IR_FORCEINLINE explicit operator bool() const
        {
            return value_ & ShiftedMask;
        }

    private:
        T value_;
    };

    template <typename T, usize Index>
    class bit_field<T, Index, 1>
    {
    public:
        static constexpr T Mask = 1;
        static constexpr T ShiftedMask = Mask << Index;

        using Type = bool;

        IR_FORCEINLINE bit_field& operator=(Type value)
        {
            value_ ^= (value_ ^ (T(value) << Index)) & Mask;

            return *this;
        }

        IR_FORCEINLINE operator Type() const
        {
            return value_ & ShiftedMask;
        }

    private:
        T value_;
    };
} // namespace Iridium
