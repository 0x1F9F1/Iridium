#pragma once

#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    struct DaveHeader
    {
        u32 Magic;
        u32 FileCount;
        u32 NamesOffset;
        u32 NamesSize;
    };

    struct DaveEntry
    {
        u32 NameOffset;
        u32 DataOffset;
        u32 Size;
        u32 RawSize;

        u32 GetNameOffset() const
        {
            return NameOffset;
        }

        u32 GetOnDiskSize() const
        {
            return RawSize;
        }

        u32 GetOffset() const
        {
            return DataOffset;
        }

        u32 GetSize() const
        {
            return Size;
        }
    };

    class DaveArchive final : public FileDevice
    {
    public:
        DaveArchive(Rc<Stream> stream);

        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

        static void Save(Rc<FileDevice> device, Vec<String> files, Rc<Stream> output);

    private:
        bool RefreshFileList();

        Rc<Stream> OpenEntry(StringView path, DaveEntry& entry);

        VirtualFileSystem<DaveEntry> vfs_;
        Rc<Stream> input_;
    };
} // namespace Iridium
