#pragma once

#include "asset/filedeviceext.h"

namespace Iridium::RSC
{
    struct datResourceInfo
    {
        u32 VirtualFlags {0};
        u32 PhysicalFlags {0};
    };

    struct datResourceFileHeader
    {
        u32 Magic {0};
        u32 Flags {0};
        datResourceInfo ResourceInfo;
    };

    struct fiResourceInfo
    {
        datResourceFileHeader FileHeader;
        u64 OnDiskSize;
        i64 FileTime;
    };

    struct datResourceChunk
    {
        u64 Vaddr;
        u64 Size;
    };

    struct ResourceFileHeaderExtension : FileDeviceExtensionT<ResourceFileHeaderExtension>
    {
        StringView FileName;
        datResourceFileHeader Info;

        static constexpr StringView ExtensionName = "ResourceFileHeader";
    };
} // namespace Iridium::RSC

namespace Iridium::RSC7
{
    using namespace RSC;

    u32 GetChunkCount(u32 flags);
    u32 GetLargeResourceSize(u8 header[16]);

    u64 GetResourceSize(u32 flags, u32 chunk_size);

    Vec<datResourceChunk> GetChunkMap(u32 flags, u32 chunk_size, u64 vaddr, u64& total);
} // namespace Iridium::RSC7
