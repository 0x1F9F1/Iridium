#include "rpf6.h"

#include "core/bits.h"

#include "asset/stream.h"
#include "asset/stream/buffered.h"
#include "asset/stream/partial.h"

#include "crypto/aes.h"

namespace Iridium::Rage
{
    class RageCipher16 : public Cipher
    {
    public:
        RageCipher16(Ptr<Cipher> cipher)
            : cipher_(std::move(cipher))
        {}

        virtual usize Update(const u8* input, u8* output, usize length) override
        {
            for (usize i = 0; i < 16; ++i)
                length = cipher_->Update(input, output, length);

            return length;
        }

        virtual usize GetBlockSize() override
        {
            return cipher_->GetBlockSize();
        }

    private:
        Ptr<Cipher> cipher_;
    };

    static u8 RDR1_PLATFORM_KEY[32];

    static std::unordered_map<u32, String> KnownFiles_RDR1;

    bool PackFile6::LoadDecryptionKeys(BufferedStream& input)
    {
        return input.Read(RDR1_PLATFORM_KEY, sizeof(RDR1_PLATFORM_KEY)) == sizeof(RDR1_PLATFORM_KEY);
    }

    void PackFile6::LoadFileList(BufferedStream& input)
    {
        for (String line; input.GetLine(line); line.clear())
        {
            u32 hash = StringHash::HashIdent(line).Value;

            KnownFiles_RDR1.try_emplace(hash, line);
        }
    }

    static String GetFileName(u32 hash, bool is_directory)
    {
        if (auto find = KnownFiles_RDR1.find(hash); find != KnownFiles_RDR1.end())
        {
            return find->second;
        }

        return fmt::format("{:08X}{}", hash, is_directory ? "" : ".bin");
    }

    PackFile6::PackFile6(Rc<Stream> input)
        : input_(std::move(input))
    {
        RefreshFileList();
    }

    Rc<Stream> PackFile6::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, fiPackEntry6& entry) { return OpenEntry(path, entry); });
    }

    Ptr<FindFileHandle> PackFile6::Find(StringView path)
    {
        return vfs_.Find(path, [](const fiPackEntry6& entry, FolderEntry& output) { output.Size = entry.GetSize(); });
    }

    bool PackFile6::RefreshFileList()
    {
        if (!input_->TrySeek(0))
            return false;

        if (!input_->TryRead(&header_, sizeof(header_)))
            return false;

        bool swap_endian = false;

        if (header_.Magic == 0x36465052)
        {
            bits::bswap(header_.Magic, header_.EntryCount, header_.NamesOffset, header_.DecryptionTag);

            swap_endian = true;
        }

        if (header_.Magic != 0x52504636)
            return false;

        entries_.resize(header_.EntryCount);

        if (!input_->TryRead(entries_.data(), entries_.size() * sizeof(fiPackEntry6)))
            return false;

        if (Option<Ptr<Cipher>> cipher = MakeCipher())
        {
            if (*cipher)
                (*cipher)->Update(entries_.data(), entries_.size() * sizeof(fiPackEntry6));
        }
        else
        {
            return false;
        }

        if (swap_endian)
        {
            for (fiPackEntry6& entry : entries_)
            {
                bits::bswap(entry.dword0, entry.dword4, entry.dword8, entry.dwordC, entry.dword10);
            }
        }

        String path;
        AddFile(entries_, 0, path);

        return true;
    }

    void PackFile6::AddFile(const Vec<fiPackEntry6>& entries, u32 index, String& path)
    {
        usize old_size = path.size();

        const fiPackEntry6& entry = entries.at(index);

        if (index)
            path += GetFileName(entry.GetHash(), entry.IsDirectory());

        if (entry.IsDirectory())
        {
            if (index)
                path.push_back('/');

            for (u32 i = entry.GetEntryIndex(), end = i + entry.GetEntryCount(); i < end; ++i)
            {
                AddFile(entries, i, path);
            }
        }
        else
        {
            vfs_.AddFile(path, entry);
        }

        path.resize(old_size);
    }

    Rc<Stream> PackFile6::OpenEntry(StringView /*path*/, fiPackEntry6& entry)
    {
        u64 offset = entry.GetOffset();
        u32 raw_size = entry.GetOnDiskSize();

        return MakeRc<PartialStream>(offset, raw_size, input_);
    }

    Option<Ptr<Cipher>> PackFile6::MakeCipher()
    {
        u32 tag = header_.DecryptionTag;

        if (tag == 0)
            return nullptr;

        if (tag == 0xFFFFFFFD)
        {
            return MakeUnique<RageCipher16>(
                MakeUnique<AesEcbCipher>(RDR1_PLATFORM_KEY, sizeof(RDR1_PLATFORM_KEY), true));
        }

        // fmt::print("Invalid Decryption Tag 0x{:08X}", header_.DecryptionTag);

        return None;
    }

    template class VirtualFileSystem<fiPackEntry6>;
} // namespace Iridium::Rage
