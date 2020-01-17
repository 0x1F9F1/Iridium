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

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    protected:
        Rc<Stream> input_;

        void ReserveFiles(usize count);
        void AddFile(StringView name, i64 offset, i64 size, i64 raw_size, CompressorId compression);
        void Finalize();

    private:
        struct BasicFileEntry
        {
            i64 Offset {0};
            i64 Size {0};
            i64 RawSize {0};

            CompressorId Compression {};
        };

        VirtualFileSystem<BasicFileEntry> vfs_;
    };

    extern template class VirtualFileSystem<FileArchive::BasicFileEntry>;
} // namespace Iridium
