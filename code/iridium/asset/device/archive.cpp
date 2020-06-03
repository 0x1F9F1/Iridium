#include "archive.h"

#include "asset/filedevice.h"
#include "asset/findhandle.h"
#include "asset/stream.h"

#include "asset/stream/decode.h"
#include "asset/stream/partial.h"

#include "asset/transform/deflate.h"

namespace Iridium
{
    FileArchive::FileArchive(Rc<Stream> input)
        : input_(std::move(input))
    {}

    FileArchive::~FileArchive() = default;

    Rc<Stream> FileArchive::Open(StringView path, bool read_only)
    {
        return vfs_.Open(
            path, read_only, [this](StringView path, BasicFileEntry& entry) { return OpenEntry(path, entry); });
    }

    Rc<Stream> FileArchive::Create(StringView path, bool /*write_only*/, bool truncate)
    {
        return vfs_.Create(
            path, truncate, [this](StringView path, BasicFileEntry& entry) { return OpenEntry(path, entry); });
    }

    bool FileArchive::Exists(StringView path)
    {
        return vfs_.Exists(path);
    }

    Ptr<FindFileHandle> FileArchive::Find(StringView path)
    {
        return vfs_.Find(path, [](const BasicFileEntry& entry, FolderEntry& output) { output.Size = entry.Size; });
    }

    void FileArchive::ReserveFiles(usize count)
    {
        vfs_.Reserve(count);
    }

    void FileArchive::AddFile(StringView name, i64 offset, i64 size, i64 raw_size, CompressorId compression)
    {
        if (compression == CompressorId::Stored)
            IrAssert(size == raw_size, "Stored file size and raw size cannot differ");

        vfs_.AddFile(name, BasicFileEntry {offset, size, raw_size, compression});
    }

    void FileArchive::Finalize()
    {
        // vfs_.CompactNames();
    }

    Rc<Stream> FileArchive::OpenEntry(StringView /*path*/, BasicFileEntry& entry)
    {
        switch (entry.Compression)
        {
            case CompressorId::Stored: return MakeRc<PartialStream>(entry.Offset, entry.Size, input_);

            case CompressorId::Deflate:
                return MakeRc<DecodeStream>(MakeRc<PartialStream>(entry.Offset, entry.RawSize, input_),
                    MakeUnique<InflateTransform>(), entry.Size);
        }

        return nullptr;
    }
} // namespace Iridium
