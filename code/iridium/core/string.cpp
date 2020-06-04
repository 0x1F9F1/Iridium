#include "string.h"

namespace Iridium
{
    String Concat(std::initializer_list<StringView> values)
    {
        String result;

        usize total = 0;

        for (StringView v : values)
            total += v.size();

        result.reserve(total);

        for (StringView v : values)
            result += v;

        return result;
    }

    bool StartsWith(StringView haystack, StringView needle)
    {
        return haystack.size() >= needle.size() && haystack.compare(0, needle.size(), needle) == 0;
    }

    bool EndsWith(StringView haystack, StringView needle)
    {
        return haystack.size() >= needle.size() &&
            haystack.compare(haystack.size() - needle.size(), needle.size(), needle) == 0;
    }
} // namespace Iridium
