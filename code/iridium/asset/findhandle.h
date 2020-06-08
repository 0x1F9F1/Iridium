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

    class FindFileIterator;

    class FindFileHandle
    {
    public:
        virtual ~FindFileHandle() = default;
        virtual bool Next(FolderEntry& entry) = 0;

        Vec<String> GetFileNames();

        FindFileIterator begin();
        std::nullptr_t end();
    };

    class FindFileIterator
    {
    public:
        inline FindFileIterator(FindFileHandle* handle)
            : handle_(handle)
        {
            ++(*this);
        }

        inline bool operator!=(std::nullptr_t) const
        {
            return handle_ != nullptr;
        }

        inline FolderEntry& operator*()
        {
            return entry_;
        }

        inline void operator++()
        {
            if (handle_ && !handle_->Next(entry_))
                handle_ = nullptr;
        }

    private:
        FindFileHandle* handle_ {nullptr};
        FolderEntry entry_ {};
    };

    inline FindFileIterator FindFileHandle::begin()
    {
        return FindFileIterator(this);
    }

    inline std::nullptr_t FindFileHandle::end()
    {
        return nullptr;
    }
} // namespace Iridium
