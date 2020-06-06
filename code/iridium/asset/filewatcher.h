#pragma once

namespace Iridium
{
    class FileSystemListener
    {
    public:
        virtual ~FileSystemListener() = default;

        virtual void OnAdded(StringView name);
        virtual void OnRemoved(StringView name);
        virtual void OnModified(StringView name);
        virtual void OnRenamed(StringView from, StringView to);
    };

    namespace NotifyFilter
    {
        enum NotifyFilters : u32
        {
            FileName = 1 << 0,
            FolderName = 1 << 1,
            Attributes = 1 << 2,
            Size = 1 << 3,
            LastWrite = 1 << 4,
            LastAccess = 1 << 5,
            CreationTime = 1 << 6,

            All = 0xFFFFFFFF
        };
    }

    using NotifyFilter::NotifyFilters;

    class FileSystemWatcher
    {
    public:
        virtual ~FileSystemWatcher() = default;

        virtual bool Poll(FileSystemListener& listener, u32 timeout) = 0;
    };
} // namespace Iridium
