#pragma once

#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    class Cipher;
    class BufferedStream;
} // namespace Iridium

namespace Iridium::Rage
{
    struct fiPackHeader6
    {
        u32 Magic;
        u32 EntryCount;

        // u32 name_offset = NamesOffset * 8;
        // u32 names_size = FileSize - name_offset;
        // Pair<u32, u32> NameInfo[EntryCount]; // { NameOffset, Unknown4 }
        // char NameData[names_size - (EntryCount * sizeof(u32) * 2)];
        u32 NamesOffset;
        u32 DecryptionTag;
    };

    struct fiPackEntry6
    {
        u32 dword0;

        // OnDiskSize:32
        u32 dword4;

        // EntryIndex:31 (Directory)
        // Offset:31 (Binary)
        // ResourceId:8 (Resource)
        // Offset:23 (Resource)
        // IsDirectory:1
        u32 dword8;

        // EntryCount:30 (Directory)
        // RawSize:30 (Binary)
        // VirtualSizeBase:11 (Resource)
        // VirtualSizeShift:4 (Resource)
        // PhysicalSizeBase:11 (Resource)
        // PhysicalSizeShift:4 (Resource)
        // IsCompressed:1
        // IsResource:1
        u32 dwordC;

        // VirtualFlags:14 (Resource)
        // PhysicalFlags:14 (Resource)
        // MainChunkOffset:3 (Resource)
        // HasResourceFlags:1 (Resource)
        u32 dword10;

        u32 GetHash() const
        {
            return dword0;
        }

        u32 GetOnDiskSize() const
        {
            return dword4;
        }

        u32 GetSize() const
        {
            if (IsResource())
                return GetVirtualSize() + GetPhysicalSize();
            else
                return dwordC & 0x3FFFFFFF;
        }

        bool IsDirectory() const
        {
            return (dword8 & 0x80000000) == 0x80000000;
        }

        bool IsResource() const
        {
            return (dwordC & 0x80000000) == 0x80000000;
        }

        bool IsCompressed() const
        {
            return (dwordC & 0x40000000) == 0x40000000;
        }

        u64 GetOffset() const
        {
            if (IsResource())
                return u64(dword8 & 0x7FFFFF00) << 3;
            else
                return u64(dword8 & 0x7FFFFFFF) << 3;
        }

        // Resource
        u8 GetResourceId() const
        {
            return dword8 & 0xFF;
        }

        bool HasResourceFlags() const
        {
            return (dword10 & 0x80000000) == 0x80000000;
        }

        u32 GetVirtualSize() const
        {
            if (HasResourceFlags())
                return (dword10 & 0x3FFF) << 12;
            else
                return (dwordC & 0x7FF) << (((dwordC >> 11) & 0xF) + 8);
        }

        u32 GetPhysicalSize() const
        {
            if (HasResourceFlags())
                return ((dword10 & 0xFFFC000) >> 14) << 12; // (dword10 >> 2) & 0x3FFF000
            else
                return ((dwordC >> 15) & 0x7FF) << (((dwordC >> 26) & 0xF) + 8);
        }

        u32 GetMainChunkOffset() const
        {
            if (HasResourceFlags())
            {
                u32 vsize = GetVirtualSize();
                u32 main_chunk_size = 0x1000 << ((dword10 >> 28) & 0x7);

                if (vsize > 0x80000)
                    vsize = 0x80000;

                if (main_chunk_size <= vsize)
                    return vsize & ~((main_chunk_size << 1) - 1);
            }

            return 0;
        }

        // Directory
        u32 GetEntryIndex() const
        {
            return dword8 & 0x7FFFFFFF;
        }

        u32 GetEntryCount() const
        {
            return dwordC & 0x3FFFFFFF;
        }
    };

    class PackFile6 final : public FileDevice
    {
    public:
        PackFile6(Rc<Stream> input);

        Rc<Stream> Open(StringView path, bool read_only) override;

        Ptr<FindFileHandle> Find(StringView path) override;

        static bool LoadDecryptionKeys(BufferedStream& input);
        static void LoadFileList(BufferedStream& input);

    private:
        bool RefreshFileList();

        void AddFile(const Vec<fiPackEntry6>& entries, u32 index, String& path);

        Rc<Stream> OpenEntry(StringView path, fiPackEntry6& entry);

        Option<Ptr<Cipher>> MakeCipher();

        fiPackHeader6 header_ {};
        Vec<fiPackEntry6> entries_;

        VirtualFileSystem<fiPackEntry6> vfs_;
        Rc<Stream> input_;
    };

    extern template class VirtualFileSystem<fiPackEntry6>;
} // namespace Iridium::Rage
