#pragma once

namespace Iridium
{
    inline u32 joaat(const void* data, usize length)
    {
        u32 hash = 0;

        for (usize i = 0; i < length; i++)
        {
            hash += static_cast<const u8*>(data)[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }

        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);

        return hash;
    }
} // namespace Iridium
