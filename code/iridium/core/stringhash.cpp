#include "stringhash.h"

namespace Iridium
{
    u32 StringLiteralHash(StringView string, u32 seed)
    {
        u32 hash = seed;

        for (const char v : string)
        {
            hash += static_cast<unsigned char>(v);
            hash += hash << 10;
            hash ^= hash >> 6;
        }

        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;

        return hash;
    }

    u32 IdentLiteralHash(StringView string, u32 seed)
    {
        u32 hash = seed;

        for (const char v : string)
        {
            hash += NormalizeCaseAndSlash[static_cast<unsigned char>(v)];
            hash += hash << 10;
            hash ^= hash >> 6;
        }

        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;

        return hash;
    }

    void StringHasher::UpdateString(StringView string)
    {
        u32 hash = value_;

        for (const char v : string)
        {
            hash += static_cast<unsigned char>(v);
            hash += hash << 10;
            hash ^= hash >> 6;
        }

        value_ = hash;
    }

    void StringHasher::UpdateIdent(StringView string)
    {
        u32 hash = value_;

        for (const char v : string)
        {
            hash += NormalizeCaseAndSlash[static_cast<unsigned char>(v)];
            hash += hash << 10;
            hash ^= hash >> 6;
        }

        value_ = hash;
    }
} // namespace Iridium
