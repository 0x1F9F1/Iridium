#pragma once

namespace Iridium
{
    class Stream;
    class FindFileHandle;

    struct FileDeviceExtension;

    class FileDevice : public AtomicRefCounted
    {
    public:
        // Opens a file
        // Returns a handle to the opened file, or null on error
        virtual Rc<Stream> Open(StringView path, bool read_only);

        // Creates a file
        // Returns a handle to the new file, or null on error
        virtual Rc<Stream> Create(StringView path, bool write_only, bool truncate);

        // Checks whether a file exists
        // Returns whether or not the specified file exists
        virtual bool Exists(StringView path);

        // Enumerates files in the specified directory
        // Returns a handle for file enumeration, or null on error
        virtual Ptr<FindFileHandle> Find(StringView path);

        // Deletes a file
        // Returns whether the file was successfully deleted
        virtual bool Delete(StringView path);

        // CreateFolder
        // DeleteFolder
        // Stat
        // Rename
        // Move
        // GetPackProvider

        // TODO: Add FileDeviceWatcher

        virtual bool Extension(StringView path, FileDeviceExtension& data);
    };
} // namespace Iridium
