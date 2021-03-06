#include "dave.h"

#include "asset/path.h"
#include "asset/stream.h"
#include "asset/stream/decode.h"
#include "asset/stream/encode.h"
#include "asset/stream/partial.h"
#include "asset/transform/deflate.h"
#include "core/bits.h"

namespace Iridium::Angel
{
    struct DaveHeader
    {
        u32 Magic;
        u32 FileCount;
        u32 NamesOffset;
        u32 NamesSize;
    };

    static_assert(sizeof(DaveHeader) == 0x10);

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

    static_assert(sizeof(DaveEntry) == 0x10);

    static void ReadDaveString(const char* names, String& output)
    {
        // Modified base-64, using only 48 characters
        static constexpr char DaveStringCharSet[48 + 16 + 1] {
            "\0 #$()-./?0123456789_abcdefghijklmnopqrstuvwxyz~________________"};

        const unsigned char* name_data = reinterpret_cast<const unsigned char*>(names);

        u32 bit_index = 0;

        if (u8 first = name_data[0]; (first & 0x3F) < 0x38)
        {
            output.clear();
        }
        else
        {
            u8 index = (first & 0x7) | ((first & 0xC0) >> 3) | ((name_data[1]) << 5);
            output.resize(index);
            bit_index = 12;
        }

        while (true)
        {
            usize here = bit_index >> 3;

            u8 bits = 0;

            switch (bit_index & 0x7)
            {
                case 0 /*next: 6*/: bits = name_data[here] & 0x3F; break;
                case 2 /*next: 0*/: bits = name_data[here] >> 2; break;
                case 4 /*next: 2*/: bits = (name_data[here] >> 4) | ((name_data[here + 1] & 0x3) << 4); break;
                case 6 /*next: 4*/: bits = (name_data[here] >> 6) | ((name_data[here + 1] & 0xF) << 2); break;
            }

            bit_index += 6;

            if (bits == 0)
                break;

            output.push_back(DaveStringCharSet[bits]);
        }
    }

    static void WriteDaveString(Vec<char>& names, StringView name, StringView prev)
    {
        // clang-format off
        static constexpr u8 DaveStringLookup[256] {
             0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
             1, 48, 20,  2,  3, 20, 20, 20,  4,  5, 20, 20, 20,  6,  7,  8,
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 20, 20, 20, 20,  9,
            20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
            36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 20, 20, 20, 20, 20,
            20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
            36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 20, 20, 20, 47, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20
        };
        // clang-format on

        usize prefix = 0;

        for (usize const max_prefix = std::min<usize>(std::min(name.size(), prev.size()), 0xFF); prefix < max_prefix;
             ++prefix)
        {
            if (DaveStringLookup[u8(name[prefix])] != DaveStringLookup[u8(prev[prefix])])
                break;
        }

        u8 pending = 0;
        u32 bit_index = 0;

        if (prefix >= 2)
        {
            u8 first = (prefix & 0x7) | ((prefix & 0x18) << 3) | 0x38;
            u8 second = (prefix & 0xE0) >> 5;

            names.emplace_back(first);

            pending = second;
            bit_index = 12;
        }
        else
        {
            prefix = 0;
        }

        for (usize i = prefix; i < name.size(); ++i)
        {
            u8 bits = DaveStringLookup[u8(name[i])];

            IrDebugAssert(bits != 0, "Invalid Name");

            switch (bit_index & 0x7)
            {
                case 0: // Next: 6
                    pending = bits;
                    break;

                case 2: // Next: 0
                    pending |= bits << 2;
                    names.emplace_back(pending);
                    pending = 0;
                    break;

                case 4: // Next: 2
                    pending |= bits << 4;
                    names.emplace_back(pending);
                    pending = (bits >> 4) & 0x3;
                    break;

                case 6: // Next: 4
                    pending |= bits << 6;
                    names.emplace_back(pending);
                    pending = (bits >> 2) & 0xF;
                    break;
            }

            bit_index += 6;
        }

        if ((bit_index & 0x7) != 0)
            names.emplace_back(pending);

        names.emplace_back('\0');
    }

    DaveArchive::DaveArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        IrAssert(RefreshFileList(), "Invalid Archive");
    }

    struct DaveFileNode final : VFS::FixedFileNode<DaveEntry>
    {
        using FixedFileNode::FixedFileNode;

        Rc<Stream> Open(void* ctx, bool /*read_only*/) override
        {
            const Rc<Stream>& input = static_cast<DaveArchive*>(ctx)->GetInput();

            u32 mem_size = Entry.GetSize();
            u32 disk_size = Entry.GetOnDiskSize();
            u32 offset = Entry.GetOffset();

            if (mem_size != disk_size)
            {
                return MakeRc<DecodeStream>(
                    MakeRc<PartialStream>(offset, disk_size, input), MakeUnique<InflateTransform>(), mem_size);
            }
            else
            {
                return MakeRc<PartialStream>(offset, mem_size, input);
            }
        }

        bool Stat(void* /*ctx*/, FolderEntry& entry) override
        {
            entry.Size = Entry.GetSize();

            return true;
        }
    };

    bool DaveArchive::RefreshFileList()
    {
        DaveHeader header {};

        if (!input_->TryReadBulk(&header, sizeof(header), 0))
            return false;

        if (header.Magic != 0x45564144 && header.Magic != 0x65766144)
            return false; // TODO: Handle "locked" .ar (zip) files

        bool const packed_names = header.Magic == 0x65766144;

        Vec<DaveEntry> entries(header.FileCount);

        if (!input_->TryReadBulk(entries.data(), entries.size() * sizeof(DaveEntry), 2048))
            return false;

        Vec<char> names(header.NamesSize);

        if (!input_->TryReadBulk(names.data(), names.size(), 2048 + header.NamesOffset))
            return false;

        if (names.empty() || names.back() != '\0')
            names.emplace_back('\0');

        vfs_.Reserve(entries.size());
        vfs_.FileContext = this;

        String name;
        name.reserve(128);

        for (const DaveEntry& entry : entries)
        {
            if (entry.NameOffset >= names.size())
                return false;

            const char* name_data = &names.at(entry.NameOffset);

            if (packed_names)
            {
                ReadDaveString(name_data, name);
            }
            else
            {
                name = name_data;

                PathNormalizeSlash(name);
            }

            vfs_.AddFile<DaveFileNode>(name, entry);
        }

        return true;
    }

    void DaveArchive::Save(
        Rc<FileDevice> device, Vec<String> files, Rc<Stream> output, bool packed_names, u32 data_alignment)
    {
        // Data alignment is 0x800 (CD sector size) in original archives
        // Can be smaller for most archives, but MC2 (and maybe others) requires 0x800 alignment when paging files

        std::sort(files.begin(), files.end(), PathCompareLess);

        u32 const file_count = static_cast<u32>(files.size());

        Vec<DaveEntry> entries(file_count);

        DaveHeader header {};

        header.Magic = packed_names ? 0x65766144 : 0x45564144;
        header.FileCount = file_count;

        u32 offset = 0x800;

        u32 const toc_offset = offset;
        u32 const toc_size = static_cast<u32>(file_count * sizeof(DaveEntry));

        offset = bits::align<u32>(offset + toc_size, data_alignment);

        {
            Vec<char> name_heap;
            StringView prev_file;

            for (u32 i = 0; i < file_count; ++i)
            {
                StringView file = files[i];
                DaveEntry& entry = entries[i];

                entry.NameOffset = static_cast<u32>(name_heap.size());

                if (packed_names)
                {
                    WriteDaveString(name_heap, file, prev_file);
                }
                else
                {
                    name_heap.insert(name_heap.end(), file.begin(), file.end());
                    name_heap.emplace_back('\0');
                }

                prev_file = file;
            }

            u32 const names_offset = offset;
            u32 const names_size = static_cast<u32>(name_heap.size());

            output->WriteBulk(name_heap.data(), names_size, names_offset);

            header.NamesOffset = names_offset - 0x800;
            header.NamesSize = names_size;

            offset = bits::align<u32>(offset + names_size, data_alignment);
        }

        for (u32 i = 0; i < file_count; ++i)
        {
            StringView file = files[i];
            DaveEntry& entry = entries[i];

            Rc<Stream> input = device->Open(file, true);

            if (input == nullptr)
                continue;

            u32 data_size = 0;
            u32 raw_size = 0;

            {
                output->Seek(offset, SeekWhence::Set);
                EncodeStream encoder(output, MakeUnique<DeflateTransform>());
                data_size = static_cast<u32>(input->CopyTo(encoder));
                encoder.Flush();
                raw_size = static_cast<u32>(encoder.Size().get(0));
            }

            if (raw_size >= data_size)
            {
                output->Seek(offset, SeekWhence::Set);

                if (!input->TrySeek(0))
                {
                    input = nullptr;
                    input = device->Open(file, true);

                    if (input == nullptr)
                        continue;
                }

                data_size = static_cast<u32>(input->CopyTo(*output));
                raw_size = data_size;
            }

            entry.DataOffset = offset;
            entry.Size = data_size;
            entry.RawSize = raw_size;

            offset = bits::align<u32>(offset + raw_size, data_alignment);
        }

        output->WriteBulk(&header, sizeof(header), 0);
        output->WriteBulk(entries.data(), entries.size() * sizeof(DaveEntry), toc_offset);
    }
} // namespace Iridium::Angel
