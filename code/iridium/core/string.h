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

    bool PathCompareEqual(StringView lhs, StringView rhs);
    bool PathCompareLess(StringView lhs, StringView rhs);

    void PathNormalizeSlash(String& path);
    void PathNormalizeSlash(char* path);

    template <typename... Args>
    IR_FORCEINLINE String Concat(const Args&... args)
    {
        return Concat({StringView(args)...});
    }

    IR_FORCEINLINE constexpr char ToLower(char v) noexcept
    {
        return (v >= 'A' && v <= 'Z') ? (v + ('a' - 'A')) : v;
    }

    IR_FORCEINLINE constexpr bool CharEqualI(char lhs, char rhs) noexcept
    {
        const char x = rhs ^ lhs;

        return (x == 0) || ((x == 0x20) && (static_cast<unsigned char>((lhs | 0x20) - 0x61) < 26));
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
