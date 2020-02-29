#include "pbo.h"

#include "asset/filedevice.h"
#include "asset/findhandle.h"
#include "asset/stream.h"

#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"

#include "asset/transform/lzss.h"

#include "core/bits.h"

namespace Iridium
{
    PboArchive::PboArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        RefreshFileList();
    }

    Rc<Stream> PboArchive::Open(StringView path, bool read_only)
    {
        return vfs_.Open(path, read_only, [this](StringView path, PboEntry& entry) { return OpenEntry(path, entry); });
    }

    bool PboArchive::Exists(StringView path)
    {
        return vfs_.Exists(path);
    }

    Ptr<FindFileHandle> PboArchive::Find(StringView path)
    {
        return vfs_.Find(path, [](const PboEntry& entry, FolderEntry& output) { output.Size = entry.GetSize(); });
    }

    void ReadExtensions(BufferedStream& input)
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

    bool PboArchive::RefreshFileList()
    {
        StringHeap names;

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

        Vec<Tuple<StringHeap::Handle, u32, RawPboEntry>> entries;
        u32 data_offset = 0;

        BufferedStream stream(input_);

        if (!stream.TrySeek(0))
            return false;

        {
            String name;
            RawPboEntry entry;

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
                        if (entries.empty())
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

                entries.push_back({names.AddString(name), data_offset, entry});

                data_offset += entry.DataSize;
            }
        }

        u32 base_offset = static_cast<u32>(stream.Tell().get());

        for (const auto& [name, offset, entry] : entries)
        {
            vfs_.AddFile(names.GetString(name),
                PboEntry {
                    entry.PackingMethod, entry.OriginalSize, entry.DataSize, offset + base_offset, entry.TimeStamp});
        }

        return true;
    }

    Rc<Stream> PboArchive::OpenEntry(StringView /*path*/, PboEntry& entry)
    {
        u32 raw_size = entry.RawSize;

        if (entry.PackingMethod == 0x43707273)
        {
            if (raw_size >= 4)
                raw_size -= 4; // 32-bit checksum
        }

        Rc<Stream> result = MakeRc<PartialStream>(entry.Offset, raw_size, input_);

        switch (entry.PackingMethod)
        {
            case 0x43707273:
                result = MakeRc<DecodeStream>(std::move(result), MakeUnique<LzssTransform>(), entry.Size);
                break;
        }

        return result;
    }

    template class VirtualFileSystem<PboArchive::PboEntry>;
} // namespace Iridium
