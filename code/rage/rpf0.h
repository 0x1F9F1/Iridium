#pragma once

#include "asset/filedevice.h"
#include "asset/old_vfs.h"

namespace Iridium::Rage
{
    struct fiPackHeader0
    {
        u32 Magic;
        u32 HeaderSize;
        u32 EntryCount;

        // u32 dwordC;
        // u32 HeaderDecryptionTag;

        // Used in game.rpf for GTA IV to signal all file should be decrypted immediately (requires/assumes the whole RPF is preloaded into memory)
        // u32 BodyDecryptionTag;
    };

    struct fiPackEntry0
    {
        // NameOffset:31
        // IsDirectory:1
        u32 dword0;

        // Offset (Binary)
        // EntryIndex (Directory)
        u32 dword1;

        // OnDiskSize (Binary)
        // EntryCount (Directory)
        u32 dword2;

        // Size (Binary)
        // EntryCount (Directory)
        u32 dword3;

        u32 GetNameOffset() const
        {
            return dword0 & 0x7FFFFFFF;
        }

        u32 GetOnDiskSize() const
        {
            return dword2;
        }

        u32 GetOffset() const
        {
            return dword1;
        }

        bool IsDirectory() const
        {
            return dword0 & 0x80000000;
        }

        u32 GetSize() const
        {
            return dword3;
        }

        u32 GetEntryIndex() const
        {
            return dword1;
        }

        u32 GetEntryCount() const
        {
            return dword2;
        }
    };

    class PackFile0 final : public FileDevice
    {
    public:
        PackFile0(Rc<Stream> stream);

        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    private:
        bool RefreshFileList();

        void AddFile(const Vec<fiPackEntry0>& entries, const Vec<char>& names, u32 index, String& path);

        Rc<Stream> OpenEntry(StringView path, fiPackEntry0& entry);

        VirtualFileSystem<fiPackEntry0> vfs_;
        Rc<Stream> input_;
    };
} // namespace Iridium::Rage
