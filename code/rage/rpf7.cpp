#include "rpf7.h"

#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"
#include "asset/transform/deflate.h"
#include "crypto/stream/ecb.h"
// #include "asset/transform/lzxd.h"

#include "core/bits.h"
#include "core/joaat.h"

#include "crypto/aes.h"

#include "tfit.h"

#include "hashes.h"
#include "rsc.h"

namespace Iridium::Rage
{
    u32 GTA5_PC_TFIT_KEYS[101][20][4];
    u32 GTA5_PC_TFIT_TABLES[17][16][256];

    u8 GTA5_PS4_AES_KEYS[101][32];

    u8 GTA5_PS3_AES_KEY[32];
    u8 GTA5_360_AES_KEY[32];

    u8 LAUNCHER_AES_KEY[32];

    u8 RAGE_DEFAULT_AES_KEY[32];

    void PackFile7::LoadDecryptionKeysPC(BufferedStream& input)
    {
        input.Seek(0, SeekWhence::Set);

        for (usize i = 0; i < 101; ++i)
        {
            input.Read(&GTA5_PC_TFIT_KEYS[i], sizeof(GTA5_PC_TFIT_KEYS[i]));
        }

        Ptr<u32[/*84*/][256]> partial_tables = MakeUnique<u32[/*84*/][256]>(84);
        HashMap<u32, u32(*)[256]> table_map;

        for (usize i = 0; i < 84; ++i)
        {
            input.Read(&partial_tables[i], sizeof(partial_tables[i]));
            u32 hash = joaat(&partial_tables[i], sizeof(partial_tables[i]));
            table_map.emplace(hash, &partial_tables[i]);
        }

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                u32(*table)[256] = table_map.at(GTA5_PC_TABLE_HASHES[i][j].JHash);
                std::memcpy(&GTA5_PC_TFIT_TABLES[i][j], table, sizeof(*table));
            }
        }
    }

    bool PackFile7::FindDecryptionKeysPC(Rc<Stream> input)
    {
        BuzFinder f;

        for (usize i = 0; i < 101; ++i)
        {
            f.Add(GTA5_PC_KEY_HASHES[i], &GTA5_PC_TFIT_KEYS[i], sizeof(GTA5_PC_TFIT_KEYS[i]));
        }

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                f.Add(GTA5_PC_TABLE_HASHES[i][j], &GTA5_PC_TFIT_TABLES[i][j], sizeof(GTA5_PC_TFIT_TABLES[i][j]));
            }
        }

        return f.Find(std::move(input));
    }

    void PackFile7::SaveDecryptionKeysPC(BufferedStream& output)
    {
        for (usize i = 0; i < 101; ++i)
        {
            output.Write(&GTA5_PC_TFIT_KEYS[i], sizeof(GTA5_PC_TFIT_KEYS[i]));
        }

        HashSet<u32> seen;

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                if (seen.emplace(GTA5_PC_TABLE_HASHES[i][j].JHash).second)
                {
                    output.Write(&GTA5_PC_TFIT_TABLES[i][j], sizeof(GTA5_PC_TFIT_TABLES[i][j]));
                }
            }
        }
    }

    void PackFile7::LoadDecryptionKeysPS4(BufferedStream& input)
    {
        input.Read(GTA5_PS4_AES_KEYS, sizeof(GTA5_PS4_AES_KEYS));
    }

    void PackFile7::LoadDecryptionKeysPS3(BufferedStream& input)
    {
        input.Read(GTA5_PS3_AES_KEY, sizeof(GTA5_PS3_AES_KEY));
    }

    void PackFile7::LoadDecryptionKeys360(BufferedStream& input)
    {
        input.Read(GTA5_360_AES_KEY, sizeof(GTA5_360_AES_KEY));
    }

    void PackFile7::LoadDecryptionKeysLauncher(BufferedStream& input)
    {
        input.Read(LAUNCHER_AES_KEY, sizeof(LAUNCHER_AES_KEY));
    }

    void PackFile7::LoadDecryptionKeysDefault(BufferedStream& input)
    {
        input.Read(RAGE_DEFAULT_AES_KEY, sizeof(RAGE_DEFAULT_AES_KEY));
    }

    static inline StringView BaseName(StringView name)
    {
        const char* name_str = name.data();

        usize split = name.size();

        while (split > 0 && name_str[split - 1] != '/')
            --split;

        return {name_str + split, name.size() - split};
    }

    static inline u32 CalculateKeyIndex(StringView path, u64 file_size)
    {
        return (IdentLiteralHash(BaseName(path), 0) + u32(file_size)) % 101;
    }

    PackFile7::PackFile7(Rc<Stream> input, StringView name)
        : input_(std::move(input))
    {
        key_index_ = CalculateKeyIndex(name, input_->Size().get(0));

        RefreshFileList();
    }

    using namespace RSC7;

    Rc<Stream> PackFile7::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, fiPackEntry7& entry) { return OpenEntry(path, entry); });
    }

    bool PackFile7::Exists(StringView path)
    {
        return vfs_.Exists(path);
    }

    Ptr<FindFileHandle> PackFile7::Find(StringView path)
    {
        return vfs_.Find(path,
            [virt_chunk_size = virt_chunk_size_, phys_chunk_size = phys_chunk_size_](
                const fiPackEntry7& entry, FolderEntry& output) {
                if (entry.IsResource())
                {
                    u32 virt_flags = entry.GetVirtualFlags();
                    u32 phys_flags = entry.GetPhysicalFlags();

                    u64 virt_size = GetResourceSize(virt_flags, virt_chunk_size);
                    u64 phys_size = GetResourceSize(phys_flags, phys_chunk_size);

                    output.Size = virt_size + phys_size;
                }
                else
                {
                    output.Size = entry.GetSize();
                }
            });
    }

    bool PackFile7::RefreshFileList()
    {
        if (!input_->TrySeek(0))
            return false;

        if (!input_->TryRead(&header_, sizeof(header_)))
            return false;

        bool swap_endian = false;

        if (header_.Magic == 0x37465052)
        {
            bits::bswap(header_.Magic, header_.EntryCount, header_.NamesLength, header_.DecryptionTag);
            swap_endian = true;

            switch (header_.DecryptionTag)
            {
                case 0xFFFFFF8: // PS3
                    virt_chunk_size_ = 0x1000;
                    phys_chunk_size_ = 0x1580;
                    break;
            }
        }

        if (header_.Magic != 0x52504637)
            return false;

        if (header_.EntryCount == 0)
            return false;

        Vec<fiPackEntry7> entries(header_.EntryCount);

        if (!input_->TryRead(entries.data(), entries.size() * sizeof(fiPackEntry7)))
            return false;

        Vec<char> names(header_.GetNamesLength());

        if (!input_->TryRead(names.data(), names.size()))
            return false;

        if (Ptr<Cipher> cipher = MakeCipher(key_index_))
        {
            fiPackEntry7 root = entries[0];
            cipher->Update(&root, sizeof(root));

            if (!root.IsDirectory())
            {
                if (header_.DecryptionTag != 0xFEFFFFF && header_.DecryptionTag != 0xFFEFFFF)
                    return false;

                for (key_index_ = 0; key_index_ < 101; ++key_index_)
                {
                    if (cipher = MakeCipher(key_index_))
                    {
                        root = entries[0];
                        cipher->Update(&root, sizeof(root));

                        if (root.IsDirectory())
                            break;
                    }
                }

                if (!root.IsDirectory())
                    return false;
            }

            entries[0] = root;

            cipher->Update(entries.data() + 1, (entries.size() - 1) * sizeof(fiPackEntry7));
            cipher->Update(names.data(), names.size());
        }

        for (fiPackEntry7& entry : entries)
        {
            if (swap_endian)
            {
                bits::bswap(entry.qword0, entry.dword8, entry.dwordC);
            }

            if (!entry.IsDirectory() && !entry.IsResource() && entry.GetDecryptionTag() == 1)
            {
                entry.SetDecryptionTag(header_.DecryptionTag);
            }
        }

        String path;
        AddFile(entries, names, 0, path);

        return true;
    }

    void PackFile7::AddFile(const Vec<fiPackEntry7>& entries, const Vec<char>& names, u32 index, String& path)
    {
        usize old_size = path.size();

        const fiPackEntry7& entry = entries.at(index);

        path += &names.at(u32(entry.GetNameOffset()) << header_.GetNameShift());

        if (entry.IsDirectory())
        {
            if (index)
                path.push_back('/');

            for (u32 i = entry.GetEntryIndex(), end = i + entry.GetEntryCount(); i < end; ++i)
            {
                AddFile(entries, names, i, path);
            }
        }
        else
        {
            vfs_.AddFile(path, entry);
        }

        path.resize(old_size);
    }

    Rc<Stream> PackFile7::OpenEntry(StringView path, fiPackEntry7& entry)
    {
        i64 offset = entry.GetOffset();
        i64 size = 0;
        i64 raw_size = entry.GetOnDiskSize();

        Ptr<Cipher> cipher;

        if (raw_size == 0)
        {
            IrAssert(!entry.IsResource() && entry.GetDecryptionTag() == 0, "Invalid Entry");

            return MakeRc<PartialStream>(offset, entry.GetSize(), input_);
        }

        if (entry.IsResource())
        {
            if (raw_size == 0xFFFFFF)
            {
                u8 rsc_header[0x10];

                if (!input_->TryReadBulk(rsc_header, sizeof(rsc_header), offset))
                    return nullptr;

                raw_size = GetLargeResourceSize(rsc_header);
            }

            IrAssert(raw_size >= 0x10, "Invalid Resource");

            u32 virt_flags = entry.GetVirtualFlags();
            u32 phys_flags = entry.GetPhysicalFlags();

            u64 virt_size = GetResourceSize(virt_flags, virt_chunk_size_);
            u64 phys_size = GetResourceSize(phys_flags, phys_chunk_size_);

            size = virt_size + phys_size;

            if (entry.GetResourceId() == 9 || entry.GetResourceId() == 10) // #SC (Scripts)
            {
                cipher = MakeCipher(CalculateKeyIndex(path, raw_size));
            }

            offset += 0x10;
            raw_size -= 0x10;
        }
        else
        {
            size = entry.GetSize();

            if (entry.GetDecryptionTag() != 0)
            {
                cipher = MakeCipher(CalculateKeyIndex(path, size));
            }
        }

        Rc<Stream> result = MakeRc<PartialStream>(offset, raw_size, input_);

        if (cipher)
            result = MakeRc<EcbCipherStream>(std::move(result), std::move(cipher));

        if (header_.GetPlatformBit())
        {
            // result = MakeRc<DecodeStream>(std::move(result), MakeUnique<LzxdTransform>(), size);
        }
        else
        {
            result = MakeRc<DecodeStream>(std::move(result), MakeUnique<InflateTransform>(), size);
        }

        return result;
    }

    Ptr<Cipher> PackFile7::MakeCipher(u32 index)
    {
        u32 tag = header_.DecryptionTag;

        if (tag == 0 || tag == 0x4E45504F || tag == 0x50584643)
            return nullptr;

        u32 id = tag & 0xFFFFFFF;
        bool sixteen_rounds = (tag >> 28) == 0xF; // Repeat decryption 16 times
        // bool is_tfit = (tag & 0xFF00000) == 0xFE00000;

        IrAssert(!sixteen_rounds, "16 Rounds not supported");

        switch (id)
        {
            case 0xFEFFFFF: {
                IrAssert(index < 101, "Invalid Key Index");

                return MakeUnique<TfitEcbCipher>(GTA5_PC_TFIT_KEYS[index] + 1, GTA5_PC_TFIT_TABLES);
            }

            case 0xFFEFFFF: {
                IrAssert(index < 101, "Invalid Key Index");

                return MakeUnique<AesEcbCipher>(GTA5_PS4_AES_KEYS[index], 32, true);
            }

            case 0xFFFFFF8: return MakeUnique<AesEcbCipher>(GTA5_PS3_AES_KEY, 32, true);

            case 0xFFFFFF7: {
                if (header_.GetPlatformBit())
                    return MakeUnique<AesEcbCipher>(GTA5_360_AES_KEY, 32, true);
                else
                    return MakeUnique<AesEcbCipher>(LAUNCHER_AES_KEY, 32, true);
            }

            case 0xFFFFFF9: return MakeUnique<AesEcbCipher>(RAGE_DEFAULT_AES_KEY, 32, true);
        }

        IrAssert(false, "Unknown Cipher Tag");
    }

    template class VirtualFileSystem<fiPackEntry7>;
} // namespace Iridium::Rage
