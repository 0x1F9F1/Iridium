#pragma once

#ifdef _MSC_VER
#    include <stdlib.h>
#else
#    error "Unsupported Platform"
#endif

namespace Iridium::bits
{
    template <typename... Args>
    IR_FORCEINLINE void bswap(Args&... args) noexcept;

    template <typename T>
    struct packed
    {
        u8 data[sizeof(T)] {};

        IR_FORCEINLINE operator T() const noexcept
        {
            T value;
            std::memcpy(&value, data, sizeof(value));
            return value;
        }

        IR_FORCEINLINE packed& operator=(const T& value) noexcept
        {
            std::memcpy(data, &value, sizeof(value));
        }
    };

    template <typename T>
    IR_FORCEINLINE constexpr T align(T value, T alignment) noexcept
    {
        return (value + alignment - 1) & ~T(alignment - 1);
    }

    template <typename T>
    using le = T;

    template <typename T>
    using ple = packed<le<T>>;

    template <typename... Args>
    IR_FORCEINLINE void bswap(Args&... args) noexcept
    {
        (bswap<Args>(args), ...);
    }

    // TODO: Add i8, i16, i32, i64, f32, f64

    template <>
    IR_FORCEINLINE void bswap<u8>(u8&) noexcept
    {
        // Do Nothing
    }

    template <>
    IR_FORCEINLINE void bswap<u16>(u16& value) noexcept
    {
#ifdef _MSC_VER
        value = _byteswap_ushort(value);
#else
#    error "Unsupported Platform"
#endif
    }

    template <>
    IR_FORCEINLINE void bswap<u32>(u32& value) noexcept
    {
#ifdef _MSC_VER
        value = _byteswap_ulong(value);
#else
#    error "Unsupported Platform"
#endif
    }

    template <>
    IR_FORCEINLINE void bswap<u64>(u64& value) noexcept
    {
#ifdef _MSC_VER
        value = _byteswap_uint64(value);
#else
#    error "Unsupported Platform"
#endif
    }
} // namespace Iridium::bits
