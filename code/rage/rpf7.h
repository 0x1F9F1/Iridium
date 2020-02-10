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
    struct fiPackHeader7
    {
        u32 Magic;
        u32 EntryCount;
        u32 NamesLength;
        u32 DecryptionTag;

        u32 GetNamesLength() const
        {
            return NamesLength & 0xFFFFFFF;
        }

        bool GetPlatformBit() const
        {
            return NamesLength & 0x80000000;
        }

        u8 GetNameShift() const
        {
            return (NamesLength >> 28) & 0x7;
        }
    };

    struct fiPackEntry7
    {
        // NameOffset:16
        // OnDiskSize:24
        // Offset:23
        // IsResource:1
        u64 qword0;

        // Size (Binary)
        // VirtualFlags (Resource)
        // EntryIndex (Directory)
        u32 dword8;

        // DecryptionTag (Binary)
        // PhysicalFlags (Resource)
        // EntryCount (Directory)
        u32 dwordC;

        u16 GetNameOffset() const
        {
            return qword0 & 0xFFFF;
        }

        u32 GetOnDiskSize() const
        {
            return (qword0 >> 16) & 0xFFFFFF;
        }

        u32 GetOffset() const
        {
            return ((qword0 >> 40) & 0x7FFFFF) << 9;
        }

        bool IsDirectory() const
        {
            return GetOffset() == 0xFFFFFE00;
        }

        bool IsResource() const
        {
            return (qword0 >> 63) == 1;
        }

        u32 GetVirtualFlags() const
        {
            return dword8;
        }

        u32 GetPhysicalFlags() const
        {
            return dwordC;
        }

        u32 GetDecryptionTag() const
        {
            return dwordC;
        }

        void SetDecryptionTag(u32 tag)
        {
            dwordC = tag;
        }

        u32 GetSize() const
        {
            return dword8;
        }

        u8 GetResourceId() const
        {
            u8 upper = (dword8 >> 28);
            u8 lower = (dwordC >> 28);

            return (upper << 4) | lower;
        }

        u32 GetEntryIndex() const
        {
            return dword8;
        }

        u32 GetEntryCount() const
        {
            return dwordC;
        }
    };

    class PackFile7 final : public FileDevice
    {
    public:
        PackFile7(Rc<Stream> stream, StringView name);

        Rc<Stream> Open(StringView path, bool read_only) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

        static void LoadDecryptionKeysPC(BufferedStream& input);
        static bool FindDecryptionKeysPC(Rc<Stream> input);
        static void SaveDecryptionKeysPC(BufferedStream& output);

        static void LoadDecryptionKeysPS4(BufferedStream& input);
        static void LoadDecryptionKeysPS3(BufferedStream& input);
        static void LoadDecryptionKeys360(BufferedStream& input);
        static void LoadDecryptionKeysLauncher(BufferedStream& input);
        static void LoadDecryptionKeysDefault(BufferedStream& input);

    private:
        bool RefreshFileList();

        void AddFile(const Vec<fiPackEntry7>& entries, const Vec<char>& names, u32 index, String& path);

        Rc<Stream> OpenEntry(StringView path, fiPackEntry7& entry);

        Ptr<Cipher> MakeCipher(u32 key_index);

        u32 key_index_ {0};

        u32 virt_chunk_size_ {0x2000};
        u32 phys_chunk_size_ {0x2000};

        fiPackHeader7 header_ {};
        VirtualFileSystem<fiPackEntry7> vfs_;
        Rc<Stream> input_;
    };

    extern template class VirtualFileSystem<fiPackEntry7>;
} // namespace Iridium::Rage
