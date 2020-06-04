#pragma once

namespace Iridium
{
    extern const u8 NormalizeCaseAndSlash[256];

    IR_FORCEINLINE bool PathCompareEqual(StringView lhs, StringView rhs)
    {
        usize const len = lhs.size();

        if (len != rhs.size())
            return false;

        const char* const lhs_raw = lhs.data();
        const char* const rhs_raw = rhs.data();

        for (usize i = 0; i != len; ++i)
        {
            if (NormalizeCaseAndSlash[static_cast<unsigned char>(lhs_raw[i])] !=
                NormalizeCaseAndSlash[static_cast<unsigned char>(rhs_raw[i])])
                return false;
        }

        return true;
    }

    IR_FORCEINLINE bool PathCompareLess(StringView lhs, StringView rhs)
    {
        usize const l1 = lhs.size();
        usize const l2 = rhs.size();

        const char* const lhs_raw = lhs.data();
        const char* const rhs_raw = rhs.data();

        usize const len = std::min(l1, l2);

        for (usize i = 0; i < len; ++i)
        {
            u8 const i1 = NormalizeCaseAndSlash[static_cast<unsigned char>(lhs_raw[i])];
            u8 const i2 = NormalizeCaseAndSlash[static_cast<unsigned char>(rhs_raw[i])];

            if (i1 != i2)
                return i1 < i2;
        }

        return l1 < l2;
    }

    IR_FORCEINLINE void PathNormalizeSlash(String& path)
    {
        for (char& v : path)
        {
            if (v == '\\')
                v = '/';
        }
    }

    IR_FORCEINLINE void PathNormalizeSlash(char* path)
    {
        for (; *path; ++path)
        {
            if (*path == '\\')
                *path = '/';
        }
    }
} // namespace Iridium
