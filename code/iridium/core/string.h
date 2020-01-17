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
    inline String Concat(const Args&... args)
    {
        return Concat({StringView(args)...});
    }

    inline constexpr char ToLower(char v) noexcept
    {
        return (v >= 'A' && v <= 'Z') ? (v + ('a' - 'A')) : v;
    }

    extern const u8 NormalizeCaseAndSlash[256];
} // namespace Iridium

[[nodiscard]] inline Iridium::String operator"" _s(const char* str, std::size_t len)
{
    return Iridium::String(str, len);
}

[[nodiscard]] inline constexpr Iridium::StringView operator"" _sv(const char* str, std::size_t len)
{
    return Iridium::StringView(str, len);
}
