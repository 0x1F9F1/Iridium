#pragma once

#include "asset/compressorid.h"
#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    class FileArchive : public FileDevice
    {
    public:
        FileArchive(Rc<Stream> input);

        Rc<Stream> Open(StringView path, bool read_only) override;
        Rc<Stream> Create(StringView path, bool write_only, bool truncate) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    protected:
        struct BasicFileEntry
        {
            i64 Offset {0};
            i64 Size {0};
            i64 RawSize {0};

            CompressorId Compression {};
        };

        Rc<Stream> input_;

        using VFS = VirtualFileSystem<BasicFileEntry>;

        VFS vfs_;

        void ReserveFiles(usize count);
        void AddFile(StringView name, i64 offset, i64 size, i64 raw_size, CompressorId compression);
        void Finalize();

    private:
        Rc<Stream> OpenEntry(StringView path, BasicFileEntry& entry);
    };
} // namespace Iridium
