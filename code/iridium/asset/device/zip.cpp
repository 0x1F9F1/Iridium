#include "zip.h"

#include "asset/path.h"
#include "asset/stream.h"
#include "asset/stream/buffered.h"
#include "asset/stream/decode.h"
#include "asset/stream/partial.h"
#include "asset/transform/deflate.h"
#include "core/bits.h"

namespace Iridium::Zip
{
    // http://www.pkware.com/documents/casestudies/APPNOTE.TXT

    using namespace bits;

    struct ZIPENDLOCATOR
    {
        ple<u32> Signature; // 0x06054B50
        ple<u16> DiskNumber;
        ple<u16> StartDiskNumber;
        ple<u16> EntriesOnDisk;
        ple<u16> EntriesInDirectory;
        ple<u32> DirectorySize;
        ple<u32> DirectoryOffset;
        ple<u16> CommentLength;
    };

    static_assert(sizeof(ZIPENDLOCATOR) == 0x16);

    struct ZIP64ENDLOCATOR
    {
        ple<u32> Signature; // 0x07064B50
        ple<u32> StartDiskNumber;
        ple<i64> DirectoryOffset;
        ple<u32> EntriesInDirectory;
    };

    static_assert(sizeof(ZIP64ENDLOCATOR) == 0x14);

    struct ZIP64ENDLOCATORRECORD
    {
        ple<u32> Signature; // 0x06064B50
        ple<i64> DirectoryRecordSize;
        ple<u16> VersionMadeBy;
        ple<u16> VersionToExtract;
        ple<u32> DiskNumber;
        ple<u32> StartDiskNumber;
        ple<i64> EntriesOnDisk;
        ple<i64> EntriesInDirectory;
        ple<i64> DirectorySize;
        ple<i64> DirectoryOffset;
    };

    static_assert(sizeof(ZIP64ENDLOCATORRECORD) == 0x38);

    struct ZIPDIRENTRY
    {
        ple<u32> Signature; // 0x02014B50
        ple<u16> VersionMadeBy;
        ple<u16> VersionToExtract;
        ple<u16> Flags;
        ple<u16> Compression;
        ple<u16> FileTime;
        ple<u16> FileDate;
        ple<u32> CRC;
        ple<u32> CompressedSize;
        ple<u32> UncompressedSize;
        ple<u16> FileNameLength;
        ple<u16> ExtraFieldLength;
        ple<u16> FileCommentLength;
        ple<u16> DiskNumberStart;
        ple<u16> InternalAttributes;
        ple<u32> ExternalAttributes;
        ple<u32> HeaderOffset;
    };

    static_assert(sizeof(ZIPDIRENTRY) == 0x2E);

    struct ZIPFILERECORD
    {
        ple<u32> Signature; // 0x04034B50
        ple<u16> Version;
        ple<u16> Flags;
        ple<u16> Compression;
        ple<u16> FileTime;
        ple<u16> FileDate;
        ple<u32> CRC;
        ple<u32> CompressedSize;
        ple<u32> UncompressedSize;
        ple<u16> FileNameLength;
        ple<u16> ExtraFieldLength;
    };

    static_assert(sizeof(ZIPFILERECORD) == 0x1E);

    struct ZIPEXTRAFIELD
    {
        ple<u16> HeaderId;
        ple<u16> DataSize;
    };

    static_assert(sizeof(ZIPEXTRAFIELD) == 0x4);
} // namespace Iridium::Zip

namespace Iridium
{
    using namespace Zip;

    ZipArchive::ZipArchive(Rc<Stream> input)
        : input_(std::move(input))
    {
        RefreshFileList();
    }

    bool ZipArchive::RefreshFileList()
    {
        return ParseCentralDirectory();
    }

    bool ZipArchive::FindEndOfCentralDirectory()
    {
        i64 const size = input_->Size().get();

        if (size < sizeof(ZIPENDLOCATOR))
            return false;

        i64 here = size - sizeof(ZIPENDLOCATOR); // Highest possible EOCD offset

        { // Fast path for files with no comment
            ZIPENDLOCATOR eocd;

            if (!input_->TryReadBulk(&eocd, sizeof(ZIPENDLOCATOR), here))
                return false;

            if (eocd.Signature == 0x06054B50)
            {
                eocd_offset_ = here;

                return true;
            }
        }

        u8 buffer[4096 + 3];

        for (i64 const lower = std::max<i64>(0, here - 0xFFFF); here > lower;)
        {
            usize len = static_cast<usize>(std::min<i64>(here - lower, sizeof(buffer) - 3));

            here -= len;

            if (!input_->TryReadBulk(buffer, len + 3, here))
                break;

            for (usize j = len; j--;)
            {
                if ((buffer[j] == 0x50) && (buffer[j + 1] == 0x4B) && (buffer[j + 2] == 0x05) &&
                    (buffer[j + 3] == 0x06))
                {
                    eocd_offset_ = here + j;

                    return true;
                }
            }
        }

        return false;
    }

    bool ZipArchive::FindEndOfCentralDirectory64()
    {
        if (eocd_offset_ < i64(sizeof(ZIP64ENDLOCATOR) + sizeof(ZIP64ENDLOCATORRECORD)))
            return false;

        ZIP64ENDLOCATOR eocd64;

        if (!input_->TryReadBulk(&eocd64, sizeof(eocd64), eocd_offset_ - sizeof(eocd64)))
            return false;

        if (eocd64.Signature != 0x07064B50)
            return false;

        eocd64_offset_ = eocd64.DirectoryOffset;

        return true;
    }

    bool ZipArchive::FindCentralDirectory()
    {
        if (!FindEndOfCentralDirectory())
            return false;

        ZIPENDLOCATOR eocd;

        if (!input_->TryReadBulk(&eocd, sizeof(eocd), eocd_offset_))
            return false;

        if (eocd.Signature != 0x06054B50)
            return false;

        if (eocd.DiskNumber != eocd.StartDiskNumber)
            return false;

        if (!FindCentralDirectory64())
        {
            cd_offset_ = eocd.DirectoryOffset;
            cd_size_ = eocd.DirectorySize;
            cd_entries_ = eocd.EntriesOnDisk;
        }

        return true;
    }

    bool ZipArchive::FindCentralDirectory64()
    {
        if (!FindEndOfCentralDirectory64())
            return false;

        ZIP64ENDLOCATORRECORD eocd64;

        if (!input_->TryReadBulk(&eocd64, sizeof(eocd64), eocd64_offset_))
            return false;

        if (eocd64.Signature != 0x06064B50)
            return false;

        if (eocd64.DiskNumber != eocd64.StartDiskNumber)
            return false;

        if (eocd64.DirectoryRecordSize + 12 < i64(sizeof(eocd64)))
            return false;

        cd_offset_ = eocd64.DirectoryOffset;
        cd_size_ = eocd64.DirectorySize;
        cd_entries_ = eocd64.EntriesOnDisk;

        return true;
    }

    struct ZipFileEntry
    {
        i64 Offset {-1};
        i64 Size {0};
        i64 RawSize {0};

        i64 HeaderOffset {0};

        CompressorId Compression {};
    };

    struct ZipFileNode final : VFS::FixedFileNode<ZipFileEntry>
    {
        using FixedFileNode::FixedFileNode;

        Rc<Stream> Open(void* ctx, bool /*read_only*/) override
        {
            const Rc<Stream>& input = static_cast<ZipArchive*>(ctx)->GetInput();

            if (Entry.Offset == -1)
            {
                ZIPFILERECORD record;

                if (!input->TryReadBulk(&record, sizeof(record), Entry.HeaderOffset))
                    return nullptr;

                if (record.Signature != 0x04034B50)
                    return nullptr;

                Entry.Offset = Entry.HeaderOffset + sizeof(record) + record.FileNameLength + record.ExtraFieldLength;
            }

            switch (Entry.Compression)
            {
                case CompressorId::Stored: return MakeRc<PartialStream>(Entry.Offset, Entry.Size, input);

                case CompressorId::Deflate:
                    return MakeRc<DecodeStream>(MakeRc<PartialStream>(Entry.Offset, Entry.RawSize, input),
                        MakeUnique<InflateTransform>(), Entry.Size);

                default: return nullptr;
            }
        }

        bool Stat(void* /*ctx*/, FolderEntry& entry) override
        {
            entry.Size = Entry.Size;

            return true;
        }
    };

    bool ZipArchive::ParseCentralDirectory()
    {
        if (!FindCentralDirectory())
            return false;

        BufferedStream stream(input_);

        if (!stream.TrySeek(cd_offset_))
            return false;

        vfs_.Reserve(cd_entries_);

        String entry_name;
        entry_name.reserve(128);

        while (true)
        {
            ZIPDIRENTRY entry;

            if (!stream.TryRead(&entry, sizeof(entry)))
                break;

            if (entry.Signature != 0x02014B50)
                break;

            // TODO: Handle Zip64 Files (CompressedSize > 0xFFFFFFFF || UncompresedSize > 0xFFFFFFFF)

            if (entry.FileNameLength)
            {
                entry_name.resize(entry.FileNameLength);

                if (!stream.TryRead(entry_name.data(), entry_name.size()))
                    break;

                if ((entry_name.back() != '/') && (entry_name.back() != '\\') &&
                    (entry.Compression == 0 || entry.Compression == 8))
                {
                    PathNormalizeSlash(entry_name);

                    vfs_.AddFile<ZipFileNode>(entry_name,
                        ZipFileEntry {-1, entry.UncompressedSize, entry.CompressedSize, entry.HeaderOffset,
                            (entry.Compression == 8) ? CompressorId::Deflate : CompressorId::Stored});
                }
            }

            stream.Seek(entry.ExtraFieldLength + entry.FileCommentLength, SeekWhence::Cur);
        }

        return true;
    }
} // namespace Iridium
