#pragma once

namespace Iridium
{
    class Stream;
    class FindFileHandle;

    // Initializes platform IO functionality
    bool PlatformIoInit();

    // Shuts down platform IO functionality
    void PlatformIoShutdown();

    // Opens a file
    // Returns a handle to the opened file, or null on error
    Rc<Stream> PlatformOpenFile(StringView path, bool read_only);

    // Creates a file
    // Returns a handle to the new file, or null on error
    Rc<Stream> PlatformCreateFile(StringView path, bool write_only, bool truncate);

    // Enumerates files in the specified folder
    // Returns a handle for file enumeration, or null on error
    Ptr<FindFileHandle> PlatformFindFiles(StringView path);

    // Creates a temporary file
    // Returns a handle to the temporary file, or null on error
    Rc<Stream> PlatformTempFile();

    // Creates a folder at the specified path
    // Returns whether the folder was succesfully created
    bool PlatformCreateFolder(StringView path, bool recursive);

    // Deletes the specified file
    // Returns whether the file was succesfully deleted
    bool PlatformDeleteFile(StringView path);

    // Delets the specified folder
    // Returns whether the folder was succesfully created
    bool PlatformDeleteFolder(StringView path);

    // Checks whether a file exists
    // Returns whether or not the specified file exists
    bool PlatformFileExists(StringView path);

    // Checks whether a folder exists
    // Returns whether or not the specified folder exists
    bool PlatformFolderExists(StringView path);

    String PlatformPathExpandEnvStrings(StringView path);
} // namespace Iridium
