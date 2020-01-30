#pragma once

#include "asset/device/archive.h"

namespace Iridium::Angel
{
    struct AresHeader
    {
        u32 Magic;
        u32 FileCount;
        u32 RootCount;
        u32 NamesSize;
    };

    struct VirtualFileINode
    {
        u32 dword0;
        u32 dword4;
        u32 dword8;

        u32 GetOffset() const
        {
            return dword0;
        }

        u32 GetSize() const
        {
            return dword4 & 0x7FFFFF;
        }

        u32 GetEntryIndex() const
        {
            return dword0;
        }

        u32 GetEntryCount() const
        {
            return dword4 & 0x7FFFFF;
        }

        bool IsDirectory() const
        {
            return (dword8 & 1) != 0;
        }

        u32 GetNameOffset() const
        {
            return (dword8 >> 14) & 0x3FFFF;
        }

        u32 GetExtOffset() const
        {
            return (dword4 >> 23) & 0x1FF;
        }

        u32 GetNameInteger() const
        {
            return (dword8 >> 1) & 0x1FFF;
        }
    };

    class AresArchive final : public FileDevice
    {
    public:
        AresArchive(Rc<Stream> stream);

        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

        static void Save(Rc<FileDevice> device, Vec<String> files, Rc<Stream> output);

    private:
        bool RefreshFileList();

        void AddFileNode(const Vec<VirtualFileINode>& nodes, const Vec<char>& names, u32 index, String& path);

        Rc<Stream> OpenEntry(StringView path, VirtualFileINode& entry);

        VirtualFileSystem<VirtualFileINode> vfs_;
        Rc<Stream> input_;
    };
} // namespace Iridium::Angel
