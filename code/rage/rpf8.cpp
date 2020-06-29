#include "rpf8.h"

#include "asset/stream.h"
#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"

#include "asset/transform/deflate.h"
#include "asset/transform/oodle.h"

#include "asset/glob.h"

#include "tfit.h"
#include "tfit2.h"

#include "core/bits.h"
#include "core/joaat.h"

#include "hashes.h"

#include "crypto/aes.h"

namespace Iridium::Rage
{
    template <typename T>
    inline T ipow2(size_t exp)
    {
        return T(1) << exp;
    }

    template <typename T>
    inline T ipow4(size_t exp)
    {
        return T(1) << (exp << 1);
    }

    // rage::AES::UnpackConfig
    void UnpackConfig(u16 config, i64& body_offset, i64& body_block_length, i64& body_block_stride)
    {
        u32 offset_config = config & 0b11;         // Config[0:2]
        u32 length_config = (config >> 2) & 0b111; // Config[2:5]
        u32 stride_config = (config >> 5) & 0b111; // Config[5:8]

        if (offset_config)
        {
            body_offset = ipow4<i64>(offset_config - 1) << 12; // * 4096
        }

        if (length_config)
        {
            body_block_length = ipow2<i64>(length_config - 1) << 11; // * 2048
            body_block_stride = i64(stride_config + 1) << 16;        // * 0x10000
        }
    }

    struct CipherContext
    {
        const u8* Input {nullptr};
        u8* Output {nullptr};
        i64 Offset {0};
        i64 Pending {0};
        i64 Length {0};
        u16 Config {0};
        bool ReadFromStart {false};
        bool ReadToEnd {false};

        Ptr<Cipher> Cipher;

        void Seek(i64 amount)
        {
            IrDebugAssert((amount >= 0) && (amount <= Pending), "Invalid Seek");

            Offset += amount;
            Pending -= amount;
        }

        i64 Update(i64 start_offset, i64 amount)
        {
            amount = (Pending >= amount) ? amount : (Pending & 0xFFFFFFF0);

            Cipher->Update(&Output[Offset - start_offset], static_cast<usize>(amount));

            Seek(amount);

            return amount;
        }

        void UnpackConfig(i64& body_offset, i64& body_block_length, i64& body_block_stride, bool& read_head,
            bool& read_body, bool& read_tail)
        {
            Rage::UnpackConfig(Config, body_offset, body_block_length, body_block_stride);

            if (body_block_length == 0 && body_block_stride == 0)
                read_body = false;

            if (ReadFromStart || ReadToEnd)
            {
                read_head = true;

                if (Length <= body_offset + 0x400)
                {
                    read_body = false;
                    read_tail = false;
                }
                else
                {
                    read_tail = true;
                }
            }
        }

        bool Update();
    };

    bool CipherContext::Update()
    {
        if (Input != Output)
            std::memcpy(Output, Input, static_cast<usize>(Pending));

        i64 start_offset = Offset;
        i64 tail_offset = Length - 0x400;

        i64 body_offset = 0;
        i64 body_block_length = 0;
        i64 body_block_stride = 0;

        bool read_head = true;
        bool read_body = true;
        bool read_tail = true;

        UnpackConfig(body_offset, body_block_length, body_block_stride, read_head, read_body, read_tail);

        if (read_head && (start_offset < body_offset))
            Update(start_offset, body_offset - start_offset);

        if ((Offset < body_offset) && !Pending)
            return true;

        if (read_body && (Offset + body_block_length <= tail_offset))
        {
            i64 body_extra = Offset % body_block_stride;

            if (!body_block_stride || !body_extra)
            {
                body_extra = (body_offset <= Offset) ? body_block_stride : 0;
            }

            i64 body_first_length = body_block_stride - body_extra;
            i64 body_first_end = Offset + body_first_length;

            if ((body_first_end < Length) || ((body_extra == 0) && (Offset != body_offset)))
            {
                bool large_body = false;
                i64 body_align = body_block_length - body_extra;

                if ((body_block_length > body_extra) && (Offset > body_block_stride))
                {
                    large_body = true;
                }
                else if (Pending <= body_first_length)
                {
                    Seek(Pending);

                    return true;
                }
                else
                {
                    if (read_tail && (body_first_end > tail_offset))
                    {
                        body_first_length = std::min<i64>(Pending, tail_offset - Offset);
                    }

                    Seek(body_first_length);
                }

                while ((Pending > 15) && (Offset + body_block_length <= tail_offset))
                {
                    i64 local0 =
                        Update(start_offset, (large_body && (body_align > 0)) ? body_align : body_block_length);

                    if (Pending < body_block_stride)
                    {
                        if (Offset + Pending < tail_offset)
                        {
                            Seek(Pending);
                        }

                        break;
                    }

                    if (large_body && (body_align > 0))
                    {
                        body_align -= local0;
                    }

                    Seek(body_block_stride - local0);
                }
            }
        }

        if ((Offset + Pending) >= tail_offset)
        {
            if (Offset < tail_offset)
            {
                Seek(tail_offset - Offset);
            }

            if (read_tail)
            {
                if ((Offset + Pending) == Length)
                {
                    Update(start_offset, 0x400);
                }
                else if ((Offset == tail_offset) && !ReadToEnd)
                {
                    Update(start_offset, Pending);
                }
            }
        }
        else
        {
            Seek(Pending);
        }

        return true;
    }

    class RageStridedCipher final : public Cipher
    {
    public:
        RageStridedCipher(u16 config, i64 size, Ptr<Cipher> cipher)
        {
            Context.Config = config;
            Context.Cipher = std::move(cipher);
            Context.Length = size;
        }

        usize Update(const u8* input, u8* output, usize length) override
        {
            Context.Input = input;
            Context.Output = output;
            Context.Pending = length;

            Context.ReadFromStart = Context.Offset == 0;
            Context.ReadToEnd = Context.Offset + Context.Pending == Context.Length;

            return Context.Update() ? length : 0;
        }

        usize GetBlockSize() override
        {
            return 1;
        }

        CipherContext Context;
    };

    class RageStridedStream final : public Stream
    {
    public:
        RageStridedStream(Rc<Stream> input, Ptr<RageStridedCipher> cipher)
            : input_(std::move(input))
            , cipher_(std::move(cipher))
        {}

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;

    private:
        Rc<Stream> input_;
        Ptr<RageStridedCipher> cipher_;
    };

    StreamPosition RageStridedStream::Tell()
    {
        return cipher_->Context.Offset;
    }

    StreamPosition RageStridedStream::Size()
    {
        return cipher_->Context.Length;
    }

    usize RageStridedStream::Read(void* ptr, usize len)
    {
        if (cipher_->Context.Offset + i64(len) < cipher_->Context.Length)
            IrAssert((len & 0xF) == 0, "Cannot read between blocks");

        return (cipher_->Cipher::Update)(ptr, input_->Read(ptr, len));
    }

    static Tfit2Context RDR2_PC_CONTEXT;
    static u64 RDR2_PC_KEYS[164][18][2];
    static u8 RDR2_PC_IV[16];

    static u32 RDR2_ANDROID_KEYS[164][20][4];
    static u32 RDR2_ANDROID_TABLES[17][16][256];
    static u8 RDR2_ANDROID_IV[16];

    static u32 RDR2_PS4_KEYS[164][20][4];
    static u32 RDR2_PS4_TABLES[17][16][256];
    static u8 RDR2_PS4_IV[16];

    bool PackFile8::LoadDecryptionKeysPC(BufferedStream& input)
    {
        input.Seek(0, SeekWhence::Set);

        for (usize i = 0; i < 164; ++i)
        {
            if (!input.TryRead(&RDR2_PC_KEYS[i], sizeof(RDR2_PC_KEYS[i])))
                return false;
        }

        if (!input.TryRead(&RDR2_PC_IV, sizeof(RDR2_PC_IV)))
            return false;

        RDR2_PC_CONTEXT.Load(input);

        return input.Tell() == input.Size();
    }

    bool PackFile8::FindDecryptionKeysPC(Rc<Stream> input)
    {
        BuzFinder f;

        for (usize i = 0; i < 164; ++i)
        {
            f.Add(RDR2_PC_KEY_HASHES[i], &RDR2_PC_KEYS[i], sizeof(RDR2_PC_KEYS[i]));
        }

        f.Add(RDR2_IV_HASH, &RDR2_PC_IV, sizeof(RDR2_PC_IV));

        RDR2_PC_CONTEXT.Find(f, RDR2_PC_TFIT2_HASHES);

        return f.Find(std::move(input));
    }

    bool PackFile8::SaveDecryptionKeysPC(BufferedStream& output)
    {
        for (usize i = 0; i < 164; ++i)
        {
            if (!output.TryWrite(&RDR2_PC_KEYS[i], sizeof(RDR2_PC_KEYS[i])))
                return false;
        }

        if (!output.TryWrite(&RDR2_PC_IV, sizeof(RDR2_PC_IV)))
            return false;

        return RDR2_PC_CONTEXT.Save(output);
    }

    bool PackFile8::LoadDecryptionKeysAndroid(BufferedStream& input)
    {
        input.Seek(0, SeekWhence::Set);

        for (usize i = 0; i < 164; ++i)
        {
            if (!input.TryRead(&RDR2_ANDROID_KEYS[i], sizeof(RDR2_ANDROID_KEYS[i])))
                return false;
        }

        Ptr<u32[/*84*/][256]> partial_tables = MakeUnique<u32[/*84*/][256]>(84);
        HashMap<u32, u32(*)[256]> table_map;

        for (usize i = 0; i < 84; ++i)
        {
            if (!input.TryRead(&partial_tables[i], sizeof(partial_tables[i])))
                return false;

            u32 hash = joaat(&partial_tables[i], sizeof(partial_tables[i]));
            table_map.emplace(hash, &partial_tables[i]);
        }

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                u32(*table)[256] = table_map.at(RDR2_TFIT_TABLE_HASHES[i][j].JHash);
                std::memcpy(&RDR2_ANDROID_TABLES[i][j], table, sizeof(*table));
            }
        }

        if (!input.TryRead(&RDR2_ANDROID_IV, sizeof(RDR2_ANDROID_IV)))
            return false;

        return true;
    }

    bool PackFile8::LoadDecryptionKeysPS4(BufferedStream& input)
    {
        input.Seek(0, SeekWhence::Set);

        for (usize i = 0; i < 164; ++i)
        {
            if (!input.TryRead(&RDR2_PS4_KEYS[i], sizeof(RDR2_PS4_KEYS[i])))
                return false;
        }

        Ptr<u32[/*84*/][256]> partial_tables = MakeUnique<u32[/*84*/][256]>(84);
        HashMap<u32, u32(*)[256]> table_map;

        for (usize i = 0; i < 84; ++i)
        {
            if (!input.TryRead(&partial_tables[i], sizeof(partial_tables[i])))
                return false;

            u32 hash = joaat(&partial_tables[i], sizeof(partial_tables[i]));
            table_map.emplace(hash, &partial_tables[i]);
        }

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                u32(*table)[256] = table_map.at(RDR2_TFIT_TABLE_HASHES[i][j].JHash);
                std::memcpy(&RDR2_PS4_TABLES[i][j], table, sizeof(*table));
            }
        }

        if (!input.TryRead(&RDR2_PS4_IV, sizeof(RDR2_PS4_IV)))
            return false;

        return true;
    }

    bool PackFile8::FindDecryptionKeysPS4(Rc<Stream> input)
    {
        BuzFinder f;

        for (usize i = 0; i < 164; ++i)
        {
            f.Add(RDR2_PS4_KEY_HASHES[i], &RDR2_PS4_KEYS[i], sizeof(RDR2_PS4_KEYS[i]));
        }

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                f.Add(RDR2_TFIT_TABLE_HASHES[i][j], &RDR2_PS4_TABLES[i][j], sizeof(RDR2_PS4_TABLES[i][j]));
            }
        }

        f.Add(RDR2_IV_HASH, &RDR2_PS4_IV, sizeof(RDR2_PS4_IV));

        return f.Find(std::move(input));
    }

    bool PackFile8::SaveDecryptionKeysPS4(BufferedStream& output)
    {
        for (usize i = 0; i < 164; ++i)
        {
            if (!output.TryWrite(&RDR2_PS4_KEYS[i], sizeof(RDR2_PS4_KEYS[i])))
                return false;
        }

        HashSet<u32> seen;

        for (usize i = 0; i < 17; ++i)
        {
            for (usize j = 0; j < 16; ++j)
            {
                if (seen.emplace(RDR2_TFIT_TABLE_HASHES[i][j].JHash).second)
                {
                    if (!output.TryWrite(&RDR2_PS4_TABLES[i][j], sizeof(RDR2_PS4_TABLES[i][j])))
                        return false;
                }
            }
        }

        if (!output.TryWrite(&RDR2_PS4_IV, sizeof(RDR2_PS4_IV)))
            return false;

        return true;
    }

    static constexpr StringView BaseRageExts[14] {
        "rpf", "#mf", "#dr", "#ft", "#dd", "#td", "#bn", "#bd", "#pd", "#bs", "#sd", "#mt", "#sc", "#cs"};

    static constexpr StringView ExtraRageExts[24] {"mrf", "cut", "gfx", "#cd", "#ld", "#pmd", "#pm", "#ed", "#pt",
        "#map", "#typ", "#ch", "#ldb", "#jd", "#ad", "#nv", "#hn", "#pl", "#nd", "#vr", "#wr", "#nh", "#fd", "#as"};

    static String GetFileExt(u8 id, char platform)
    {
        String result = "bin";

        if (id >= 0 && id < 14)
            result = BaseRageExts[id];
        else if (id >= 64 && id < 87)
            result = ExtraRageExts[id - 64];

        for (char& c : result)
            if (c == '#')
                c = platform;

        return result;
    }

    static bool CompareFileExt(StringView lhs, StringView rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        for (usize i = 0; i < lhs.size(); ++i)
        {
            const char a = lhs[i];
            const char b = rhs[i];

            if ((a != b) && (b != '#'))
                return false;
        }

        return true;
    }

    static u8 GetFileExtId(StringView ext)
    {
        for (u8 i = 0; i < 14; ++i)
            if (CompareFileExt(ext, BaseRageExts[i]))
                return i;

        for (u8 i = 0; i < 24; ++i)
            if (CompareFileExt(ext, ExtraRageExts[i]))
                return i + 64;

        return 0xFF;
    }

    static Option<Ptr<Cipher>> GetRPF8Cipher(u16 platform, u16 tag)
    {
        if (tag == 0xFF)
            return nullptr;

        if (platform == 'y')
        {
            if (tag < 163)
                return MakeUnique<Tfit2CbcCipher>(RDR2_PC_KEYS[tag] + 1, RDR2_PC_IV, &RDR2_PC_CONTEXT);

            if (tag == 0xC0)
                return MakeUnique<Tfit2CbcCipher>(RDR2_PC_KEYS[163] + 1, RDR2_PC_IV, &RDR2_PC_CONTEXT);
        }
        else if (platform == 'a')
        {
            if (tag < 163)
                return MakeUnique<TfitCbcCipher>(RDR2_ANDROID_KEYS[tag] + 1, RDR2_ANDROID_TABLES, RDR2_ANDROID_IV);

            if (tag == 0xC0)
                return MakeUnique<TfitCbcCipher>(RDR2_ANDROID_KEYS[163] + 1, RDR2_ANDROID_TABLES, RDR2_ANDROID_IV);
        }
        else if (platform == 'o')
        {
            if (tag < 163)
                return MakeUnique<TfitCbcCipher>(RDR2_PS4_KEYS[tag] + 1, RDR2_PS4_TABLES, RDR2_PS4_IV);

            if (tag == 0xC0)
                return MakeUnique<TfitCbcCipher>(RDR2_PS4_KEYS[163] + 1, RDR2_PS4_TABLES, RDR2_PS4_IV);
        }

        return None;
    }

    static std::unordered_map<Tuple<u32, u8>, String, HashTuple<u32, u8>> KnownFiles_RDR2;
    static std::unordered_map<u32, String> PossibleFiles_RDR2;

    static Pair<StringView, StringView> SplitFileName(StringView name)
    {
        auto folder = name.rfind('/');

        if (folder == StringView::npos)
            folder = 0;
        else
            folder += 1;

        if (auto ext = name.rfind('.'); (ext != String::npos) && (ext >= folder))
        {
            return {name.substr(0, ext), name.substr(ext + 1)};
        }

        return {name, {}};
    }

    static void Replace(String& subject, StringView search, StringView replace)
    {
        for (usize pos = 0; (pos = subject.find(search, pos)) != String::npos; pos += replace.length())
        {
            subject.replace(pos, search.length(), replace);
        }
    }

    static Tuple<StringView, u32, u8> NormalisePath(String& path)
    {
        Replace(path, "%PLATFORM%", "x64");

        for (char& c : path)
            c = ToLower(c);

        auto [name, ext] = SplitFileName(path);

        u8 ext_id = GetFileExtId(ext);

        StringView result = (ext_id != 0xFF) ? name : path;

        u32 hash = StringHash::HashIdent(result).Value;

        return {result, hash, ext_id};
    }

    static bool AddFileName(String path)
    {
        auto [name, hash, ext] = NormalisePath(path);

        return KnownFiles_RDR2.try_emplace(Tuple<u32, u8> {hash, ext}, name).second;
    }

    static bool AddPossibleFileName(String path)
    {
        auto [name, hash, ext] = NormalisePath(path);

        return PossibleFiles_RDR2.try_emplace(hash, name).second;
    }

    void PackFile8::LoadFileList(BufferedStream& input)
    {
        for (String line; input.GetLine(line); line.clear())
        {
            AddFileName(line);
        }
    }

    void PackFile8::SaveFileList(BufferedStream& output)
    {
        for (const auto& file : KnownFiles_RDR2)
        {
            auto [hash, ext_id] = file.first;

            output.PutString(file.second);

            if (ext_id != 0xFF)
            {
                output.PutCh('.');
                output.PutString(GetFileExt(ext_id, '#'));
            }

            output.PutCh('\n');
        }
    }

    void PackFile8::LoadPossibleFileList(BufferedStream& input)
    {
        for (String line; input.GetLine(line); line.clear())
        {
            AddPossibleFileName(line);
        }
    }

    static String GetFileName(u32 hash, u8 ext, char platform)
    {
        auto find = KnownFiles_RDR2.find(Tuple<u32, u8> {hash, ext});

        if (find == KnownFiles_RDR2.end())
        {
            if (auto poss_find = PossibleFiles_RDR2.find(hash); poss_find != PossibleFiles_RDR2.end())
            {
                KnownFiles_RDR2.emplace_hint(find, Tuple<u32, u8> {hash, ext}, poss_find->second);

                // fmt::print("Added possible name: {}", poss_find->second);
            }

            return fmt::format("hash/{:08X}.{}", hash, GetFileExt(ext, platform));
        }

        String result = find->second;

        if (ext != 0xFF)
        {
            result += '.';
            result += GetFileExt(ext, platform);
        }

        return result;
    }

    PackFile8::PackFile8(Rc<Stream> input)
        : input_(std::move(input))
    {
        RefreshFileList();
    }

    Rc<Stream> PackFile8::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, fiPackEntry8& entry) { return OpenEntry(path, entry); });
    }

    Ptr<FindFileHandle> PackFile8::Find(StringView path)
    {
        return vfs_.Find(path, [](const fiPackEntry8& entry, FolderEntry& output) { output.Size = entry.GetSize(); });
    }

    bool PackFile8::RefreshFileList()
    {
        if (!input_->TrySeek(0))
            return false;

        if (!input_->TryRead(&header_, sizeof(header_)))
            return false;

        if (header_.Magic != 0x52504638)
            return false;

        entries_.resize(header_.EntryCount);

        if (!input_->TryRead(entries_.data(), entries_.size() * sizeof(fiPackEntry8)))
            return false;

        if (Option<Ptr<Cipher>> cipher = GetRPF8Cipher(header_.PlatformId, header_.DecryptionTag))
        {
            if (*cipher)
                (*cipher)->Update(entries_.data(), entries_.size() * sizeof(fiPackEntry8));
        }
        else
        {
            // fmt::print("RPF8 - Unknown Key 0x{:02X}, {}", header_.DecryptionTag, header_.PlatformId);

            entries_.clear();

            return false;
        }

        if (header_.DecryptionTag == 0xFF)
        {
            // fmt::print("Unencrypted RPF, Reading names");

            Vec<char> names(header_.NamesLength);

            if (!input_->TryReadBulk(names.data(), names.size(), input_->Size().get() - header_.NamesLength))
                return false;

            StringView names_view(names.data(), names.size());

            for (usize i = 0; i < names_view.size();)
            {
                usize next = names_view.find('\0', i);

                if (next == i || next == StringView::npos)
                    break;

                StringView current = names_view.substr(i, next - i);

                if (current.size() > 3 && AddFileName(String(current)))
                {
                    // fmt::print("Added Name: {}", current);
                }

                i = next + 1;
            }
        }

        for (const fiPackEntry8& entry : entries_)
        {
            AddFile(entry);
        }

        return true;
    }

    void PackFile8::AddFile(const fiPackEntry8& entry)
    {
        if (entry.IsDirectory())
        {
            return;
        }

        String name = GetFileName(entry.GetHash(), entry.GetFileExtId(), char(header_.PlatformId));

        vfs_.AddFile(name, entry);
    }

    Rc<Stream> PackFile8::OpenEntry(StringView /*path*/, fiPackEntry8& entry)
    {
        u64 size = entry.GetSize();
        u64 raw_size = entry.GetOnDiskSize();
        u64 offset = entry.GetOffset();

        if (entry.IsSignatureProtected())
        {
            if (raw_size < 0x100)
                return nullptr;

            raw_size -= 0x100;
        }

        if (entry.IsResource() && raw_size >= 16)
        {
            raw_size -= 16;
            offset += 16;
        }

        if (entry.GetCompressorId() == 0)
        {
            raw_size = size;
        }

        Rc<Stream> result = MakeRc<PartialStream>(offset, raw_size, input_);

        if (u8 const key = entry.GetEncryptionKeyId(); key != 0xFF)
        {
            if (Option<Ptr<Cipher>> cipher = GetRPF8Cipher(header_.PlatformId, key))
            {
                if (*cipher)
                    result = MakeRc<RageStridedStream>(std::move(result),
                        MakeUnique<RageStridedCipher>(entry.GetEncryptionConfig(), raw_size, std::move(*cipher)));
            }
            else
            {
                // fmt::print("RPF8 - Unknown Key 0x{:02X} for {}", key, path);

                return nullptr;
            }
        }

        switch (entry.GetCompressorId())
        {
            case 1: // Deflate
                result = MakeRc<DecodeStream>(std::move(result), MakeUnique<InflateTransform>(), size);
                break;

            case 2: // Oodle
                result = MakeRc<DecodeStream>(std::move(result), MakeUnique<OodleTransform>(size), size);
                break;
        }

        return result;
    }

    template class VirtualFileSystem<fiPackEntry8>;
} // namespace Iridium::Rage
