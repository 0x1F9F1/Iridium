#include "pbo.h"

#include "asset/path.h"
#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"
#include "asset/stringheap.h"
#include "asset/transform/lzss.h"
#include "core/bits.h"

namespace Iridium
{
    struct PboEntry
    {
        u32 PackingMethod {0};
        u32 Size {0};
        u32 RawSize {0};
        u32 Offset {0};

        u32 TimeStamp {0};

        u32 GetSize() const
        {
            return (PackingMethod != 0) ? Size : RawSize;
        }
    };

    PboArchive::PboArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        RefreshFileList();
    }

    static void ReadExtensions(BufferedStream& input)
    {
        String name;
        String value;

        while (true)
        {
            name.clear();
            value.clear();

            input.GetLine(name, '\0');

            if (name.empty())
                break;

            input.GetLine(value, '\0');
        }
    }

    struct PboFileNode final : VFS::FixedFileNode<PboEntry>
    {
        using FixedFileNode::FixedFileNode;

        Rc<Stream> Open(void* ctx, bool /*read_only*/) override
        {
            PboArchive* archive = static_cast<PboArchive*>(ctx);

            u32 raw_size = Entry.RawSize;

            if (Entry.PackingMethod == 0x43707273)
            {
                if (raw_size >= 4)
                    raw_size -= 4; // 32-bit checksum
            }

            Rc<Stream> result =
                MakeRc<PartialStream>(Entry.Offset + archive->GetBaseOffset(), raw_size, archive->GetInput());

            switch (Entry.PackingMethod)
            {
                case 0x43707273:
                    result = MakeRc<DecodeStream>(std::move(result), MakeUnique<LzssTransform>(), Entry.Size);
                    break;
            }

            return result;
        }

        bool Stat(void* /*ctx*/, FolderEntry& entry) override
        {
            entry.Size = Entry.GetSize();

            return true;
        }
    };

    using namespace bits;

    struct RawPboEntry
    {
        le<u32> PackingMethod;
        le<u32> OriginalSize;
        le<u32> Reserved;
        le<u32> TimeStamp;
        le<u32> DataSize;

        bool IsVers() const
        {
            return (PackingMethod == 0x56657273) && (TimeStamp == 0) && (DataSize == 0);
        }
    };

    static_assert(sizeof(RawPboEntry) == 0x14);

    bool PboArchive::RefreshFileList()
    {
        BufferedStream stream(input_);

        if (!stream.TrySeek(0))
            return false;

        String name;
        name.reserve(128);

        RawPboEntry entry;

        u32 data_offset = 0;
        bool at_start = true;

        while (true)
        {
            name.clear();

            stream.GetLine(name, '\0');

            if (!stream.TryRead(&entry, sizeof(entry)))
                break;

            if (name.empty())
            {
                if (entry.IsVers())
                {
                    if (at_start)
                    {
                        ReadExtensions(stream);
                    }
                    else
                    {
                        // fmt::print("Invalid Vers Header");
                    }

                    continue;
                }
                else
                {
                    break;
                }
            }

            PathNormalizeSlash(name);

            vfs_.AddFile<PboFileNode>(
                name, PboEntry {entry.PackingMethod, entry.OriginalSize, entry.DataSize, data_offset, entry.TimeStamp});

            data_offset += entry.DataSize;
            at_start = false;
        }

        base_offset_ = static_cast<u32>(stream.Tell().get());

        return true;
    }
} // namespace Iridium
