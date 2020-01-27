#include "dave.h"

#include "asset/stream.h"

#include "core/bits.h"

#include "asset/stream/encode.h"
#include "asset/transform/deflate.h"

namespace Iridium
{
    static void ReadDaveStringRaw(const Vec<char>& names, usize offset, u32 bit_index, String& output)
    {
        // Modified base-64, using only 48 characters
        static constexpr char DaveEncoding[48 + 16 + 1] {
            "\0 #$()-./?0123456789_abcdefghijklmnopqrstuvwxyz~!!!!!!!!!!!!!!!!"};

        while (true)
        {
            usize here = offset + (bit_index >> 3);

            u8 bits = 0;

            switch (bit_index & 0x7)
            {
                case 0: // Next: 6
                    bits = u8(names.at(here) & 0x3F);
                    break;

                case 2: // Next: 0
                    bits = u8(names.at(here)) >> 2;
                    break;

                case 4: // Next: 2
                    bits = (u8(names.at(here)) >> 4) | (u8(names.at(here + 1) & 0x3) << 4);
                    break;

                case 6: // Next: 4
                    bits = (u8(names.at(here)) >> 6) | (u8(names.at(here + 1) & 0xF) << 2);
                    break;
            }

            bit_index += 6;

            if (bits != 0)
            {
                output += DaveEncoding[bits];
            }
            else
            {
                break;
            }
        };
    }

    static void ReadDaveString(const Vec<char>& names, usize offset, String& output)
    {
        u8 bits = 0;
        u8 first = u8(names.at(offset));

        if ((first & 0x3F) < 0x38)
        {
            output.clear();
        }
        else
        {
            u8 second = u8(names.at(offset + 1));
            u8 index = (first & 0x7) | ((first & 0xC0) >> 3) | ((second & 0x7) << 5);
            output.resize(index);
            bits = 12;
        }

        ReadDaveStringRaw(names, offset, bits, output);
    }

    DaveArchive::DaveArchive(Rc<Stream> input)
        : FileArchive(std::move(input))
    {
        RefreshFileList();
    }

    using namespace bits;

    struct DaveHeader
    {
        le<u32> Magic;
        le<u32> FileCount;
        le<u32> NamesOffset;
        le<u32> NamesSize;
    };

    struct DaveEntry
    {
        le<u32> NameOffset;
        le<u32> DataOffset;
        le<u32> Size;
        le<u32> RawSize;
    };

    bool DaveArchive::RefreshFileList()
    {
        DaveHeader header {};

        if (input_->ReadBulk(&header, sizeof(header), 0) != sizeof(header))
            return false;

        if (header.Magic != 0x45564144 && header.Magic != 0x65766144)
            return false;

        bool packed_names = header.Magic == 0x65766144;

        Vec<DaveEntry> entries(header.FileCount);

        {
            usize const entries_size = entries.size() * sizeof(DaveEntry);

            if (input_->ReadBulk(entries.data(), entries_size, 2048) != entries_size)
                return false;
        }

        Vec<char> names(header.NamesSize);

        {
            usize const names_size = names.size();

            if (input_->ReadBulk(names.data(), names_size, 2048 + header.NamesOffset) != names_size)
                return false;
        }

        if (names.back() != '\0')
            names.emplace_back('\0');

        ReserveFiles(entries.size());

        String name;

        for (const DaveEntry& entry : entries)
        {
            if (packed_names)
            {
                ReadDaveString(names, entry.NameOffset, name);
            }
            else
            {
                name = &names.at(entry.NameOffset);
            }

            PathNormalizeSlash(name);

            AddFile(name, entry.DataOffset, entry.Size, entry.RawSize,
                entry.Size != entry.RawSize ? CompressorId::Deflate : CompressorId::Stored);
        }

        Finalize();

        return true;
    }

    void DaveArchive::Save(Rc<Stream> output)
    {
        struct Visitor : VFS::Visitor
        {
            Vec<char> Names;
            Vec<DaveEntry> Entries;

            u32 AddString(StringView value)
            {
                u32 result = static_cast<u32>(Names.size());

                Names.insert(Names.end(), value.begin(), value.end());
                Names.emplace_back('\0');

                return result;
            }

            StringView GetString(u32 offset)
            {
                return &Names.at(offset);
            }

            bool VisitFile(StringView path, VFS::FileEntry& /*entry*/) override
            {
                DaveEntry file_entry;

                file_entry.NameOffset = AddString(path);
                file_entry.DataOffset = 0;
                file_entry.Size = 0;
                file_entry.RawSize = 0;

                Entries.emplace_back(file_entry);

                return true;
            }

            bool VisitFolder(StringView /*path*/) override
            {
                return true;
            }

            void Finalize(DaveArchive* archive, Rc<Stream> output)
            {
                u32 offset = 0x800;

                DaveHeader header;

                header.Magic = 0x45564144;
                header.FileCount = static_cast<u32>(Entries.size());

                u32 const entries_offset = offset;

                offset += static_cast<u32>(Entries.size() * sizeof(DaveEntry));
                offset = (offset + 0x7FF) & ~u32(0x7FF);

                header.NamesSize = static_cast<u32>(Names.size() * sizeof(char));
                header.NamesOffset = offset;
                output->WriteBulk(Names.data(), Names.size() * sizeof(char), header.NamesOffset);
                header.NamesOffset -= 0x800;

                offset += header.NamesSize;
                offset = (offset + 0x7FF) & ~u32(0x7FF);

                for (DaveEntry& entry : Entries)
                {
                    StringView file_name = GetString(entry.NameOffset);

                    if (Rc<Stream> input = archive->Open(file_name, true))
                    {
                        u32 data_start = offset;

                        u32 data_size = 0;
                        u32 raw_size = 0;

                        {
                            output->Seek(data_start, SeekWhence::Set);
                            EncodeStream encoder(output, MakeUnique<DeflateTransform>());
                            data_size = static_cast<u32>(input->CopyTo(encoder));
                            encoder.Flush();
                            raw_size = static_cast<u32>(encoder.Size().get(0));
                        }

                        if (raw_size == data_size)
                        {
                            output->Seek(data_start, SeekWhence::Set);
                            input = archive->Open(file_name, true);
                            IrAssert(input->CopyTo(*output) == data_size, "Bad");
                        }

                        offset += raw_size;
                        offset = (offset + 0x7FF) & ~u32(0x7FF);

                        entry.DataOffset = data_start;
                        entry.Size = data_size;
                        entry.RawSize = raw_size;
                    }
                }

                std::sort(Entries.begin(), Entries.end(), [this](const DaveEntry& lhs, const DaveEntry& rhs) {
                    return PathCompareLess(GetString(lhs.NameOffset), GetString(rhs.NameOffset));
                });

                output->WriteBulk(&header, sizeof(header), 0);
                output->WriteBulk(Entries.data(), Entries.size() * sizeof(DaveEntry), entries_offset);
            }
        } visitor;

        vfs_.Visit("", visitor);

        visitor.Finalize(this, output);
    }
} // namespace Iridium
