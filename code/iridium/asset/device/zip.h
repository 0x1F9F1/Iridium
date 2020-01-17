#pragma once

#include "asset/compressorid.h"
#include "asset/filedevice.h"
#include "asset/vfs.h"

namespace Iridium
{
    class ZipArchive final : public FileDevice
    {
    public:
        ZipArchive(Rc<Stream> input);

        Rc<Stream> Open(StringView path, bool read_only) override;

        bool Exists(StringView path) override;

        Ptr<FindFileHandle> Find(StringView path) override;

    protected:
        Rc<Stream> input_;

        void ReserveFiles(usize count);
        void AddFile(StringView name, i64 header_offset, i64 size, i64 raw_size, CompressorId compression);
        void Finalize();

    private:
        void RefreshFileList();

        bool FindEndOfCentralDirectory();
        bool FindEndOfCentralDirectory64();

        bool FindCentralDirectory();
        bool FindCentralDirectory64();

        bool ParseCentralDirectory();

        i64 eocd_offset_ {-1};
        i64 eocd64_offset_ {-1};

        i64 cd_offset_ {-1};
        i64 cd_size_ {0};
        i64 cd_entries_ {0};

        struct ZipFileEntry
        {
            i64 Offset {-1};
            i64 Size {0};
            i64 RawSize {0};

            i64 HeaderOffset {0};

            CompressorId Compression {};
        };

        VirtualFileSystem<ZipFileEntry> vfs_;
    };

    extern template class VirtualFileSystem<ZipArchive::ZipFileEntry>;
} // namespace Iridium
