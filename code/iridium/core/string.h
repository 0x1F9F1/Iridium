#pragma once

#include <string>
#include <string_view>

namespace Iridium
{
    using String = std::string;
    using StringView = std::string_view;

    String Concat(std::initializer_list<StringView> values);

    bool StartsWith(StringView haystack, StringView needle);
    bool EndsWith(StringView haystack, StringView needle);

    template <typename... Args>
    IR_FORCEINLINE String Concat(const Args&... args)
    {
        return Concat({StringView(args)...});
    }

    IR_FORCEINLINE constexpr char ToLower(char v) noexcept
    {
        return v + ((static_cast<unsigned char>(v - 'A') < 26) ? 0x20 : 0);
    }

    IR_FORCEINLINE bool StringCompareIEqual(StringView lhs, StringView rhs) noexcept
    {
        usize const len = lhs.size();

        if (len != rhs.size())
            return false;

        const char* const lhs_raw = lhs.data();
        const char* const rhs_raw = rhs.data();

        for (usize i = 0; i != len; ++i)
        {
            if (ToLower(lhs_raw[i]) != ToLower(rhs_raw[i]))
                return false;
        }

        return true;
    }

    IR_FORCEINLINE bool StringCompareILess(StringView lhs, StringView rhs) noexcept
    {
        usize const lhs_len = lhs.size();
        usize const rhs_len = rhs.size();

        const char* const lhs_raw = lhs.data();
        const char* const rhs_raw = rhs.data();

        usize const len = lhs_len < rhs_len ? lhs_len : rhs_len;

        for (usize i = 0; i != len; ++i)
        {
            u8 const l = ToLower(lhs_raw[i]);
            u8 const r = ToLower(rhs_raw[i]);

            if (l != r)
                return l < r;
        }

        return lhs_len < rhs_len;
    }

    extern const u8 NormalizeCaseAndSlash[256];
} // namespace Iridium

[[nodiscard]] IR_FORCEINLINE Iridium::String operator"" _s(const char* str, std::size_t len)
{
    return Iridium::String(str, len);
}

[[nodiscard]] IR_FORCEINLINE constexpr Iridium::StringView operator"" _sv(const char* str, std::size_t len)
{
    return Iridium::StringView(str, len);
}
