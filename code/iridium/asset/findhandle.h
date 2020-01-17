#pragma once

namespace Iridium
{
    struct FolderEntry
    {
        String Name; // Local name of file/folder

        bool IsFolder : 1;

        // bool HasSize : 1;
        // bool IsCompressed : 1;
        // bool IsEncrypted : 1;
        // bool HasOnDiskSize : 1;
        // bool HasLastWriteTime : 1;

        u64 Size {0}; // In Memory/Uncompressed Size
        // u64 RawSize;  // On Disk/Compressed Size
        // u64 FileTime; // Last Modification

        void Reset()
        {
            Name.clear();
            IsFolder = false;
            Size = 0;
        }
    };

    class FindFileHandle
    {
    public:
        virtual ~FindFileHandle() = default;
        virtual bool Next(FolderEntry& entry) = 0;
    };
} // namespace Iridium
