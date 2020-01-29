#include "rpf0.h"

#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"

#include "asset/transform/deflate.h"

#include "core/bits.h"

namespace Iridium::Rage
{
    PackFile0::PackFile0(Rc<Stream> input)
        : input_(std::move(input))
    {
        IrAssert(RefreshFileList(), "Invalid Archive");
    }

    Rc<Stream> PackFile0::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, fiPackEntry0& entry) { return OpenEntry(path, entry); });
    }

    Rc<Stream> PackFile0::Create(StringView path, bool /*write_only*/, bool truncate)
    {
        return vfs_.Create(
            path, truncate, [this](StringView path, fiPackEntry0& entry) { return OpenEntry(path, entry); });
    }

    bool PackFile0::Exists(StringView path)
    {
        return vfs_.Exists(path);
    }

    Ptr<FindFileHandle> PackFile0::Find(StringView path)
    {
        return vfs_.Find(path, [](const fiPackEntry0& entry, FolderEntry& output) { output.Size = entry.GetSize(); });
    }

    bool PackFile0::RefreshFileList()
    {
        fiPackHeader0 header;

        if (input_->ReadBulk(&header, sizeof(header), 0) != sizeof(header))
            return false;

        if (header.Magic != 0x30465052)
            return false;

        Vec<fiPackEntry0> entries(header.EntryCount);

        if (input_->ReadBulk(entries.data(), entries.size() * sizeof(fiPackEntry0), 2048) !=
            entries.size() * sizeof(fiPackEntry0))
            return false;

        if (header.HeaderSize < (header.EntryCount * sizeof(fiPackEntry0)))
            return false;

        u32 names_length = header.HeaderSize - (header.EntryCount * sizeof(fiPackEntry0));

        Vec<char> names(names_length);

        if (input_->ReadBulk(names.data(), names.size(), 2048 + header.HeaderSize - names_length) != names.size())
            return false;

        if (names.back() != '\0')
            names.emplace_back('\0');

        String path;
        AddFile(entries, names, 0, path);

        return true;
    }

    void PackFile0::AddFile(const Vec<fiPackEntry0>& entries, const Vec<char>& names, u32 index, String& path)
    {
        usize old_size = path.size();

        const fiPackEntry0& entry = entries.at(index);

        path += &names.at(entry.GetNameOffset());

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

    Rc<Stream> PackFile0::OpenEntry(StringView /*path*/, fiPackEntry0& entry)
    {
        u32 mem_size = entry.GetSize();
        u32 disk_size = entry.GetOnDiskSize();
        u32 offset = entry.GetOffset();

        if (mem_size != disk_size)
        {
            return MakeRc<DecodeStream>(
                MakeRc<PartialStream>(offset, disk_size, input_), MakeUnique<InflateTransform>(), mem_size);
        }
        else
        {
            return MakeRc<PartialStream>(offset, mem_size, input_);
        }
    }
} // namespace Iridium::Rage
