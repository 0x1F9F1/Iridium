#include "asset/findhandle.h"
#include "asset/platform_io.h"
#include "asset/stream.h"

#include "core/platform/minwin.h"

namespace Iridium
{
    class Win32FileStreamBase : public Stream
    {
    public:
        Win32FileStreamBase(HANDLE handle);
        ~Win32FileStreamBase() override = 0;

        StreamPosition Seek(i64 offset, SeekWhence whence) override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;
        usize Write(const void* ptr, usize len) override;

        usize ReadBulk(void* ptr, usize len, u64 offset) override;
        usize WriteBulk(const void* ptr, usize len, u64 offset) override;

        bool Flush() override;

        StreamPosition SetSize(u64 length) override;

        bool IsBulkSync() override;
        bool IsFullSync() override;

    protected:
        HANDLE handle_ {nullptr};
    };

    class Win32FileStream final : public Win32FileStreamBase
    {
    public:
        using Win32FileStreamBase::Win32FileStreamBase;

        ~Win32FileStream() override;
    };

    class Win32TempFileStream final : public Win32FileStreamBase
    {
    public:
        using Win32FileStreamBase::Win32FileStreamBase;

        ~Win32TempFileStream() override;
    };

    class Win32FindFileHandle final : public FindFileHandle
    {
    public:
        Win32FindFileHandle(const wchar_t* path);
        ~Win32FindFileHandle() override;

        bool Next(FolderEntry& entry) override;

    private:
        HANDLE handle_ {};
        bool valid_ {false};
        WIN32_FIND_DATAW data_ {};
    };

    class Win32FindVolumeHandle final : public FindFileHandle
    {
    public:
        bool Next(FolderEntry& entry) override;

    private:
        char index_ {'A'};
    };

    class Win32TempFileCache
    {
    public:
        Win32TempFileCache();
        ~Win32TempFileCache();

        HANDLE Open();
        void Close(HANDLE handle);

        void CloseAll();

        static Win32TempFileCache Instance;

    private:
        CRITICAL_SECTION lock_;
        wchar_t temp_path_[MAX_PATH];
        usize handle_count_ {0};
        HANDLE handles_[16];
    };

    Win32TempFileCache Win32TempFileCache::Instance;

    static usize Win32ToWideChar(StringView input, wchar_t* buffer, usize buffer_len)
    {
        if (buffer_len == 0)
            return 0;

        usize converted = static_cast<usize>(MultiByteToWideChar(
            CP_UTF8, 0, input.data(), static_cast<int>(input.size()), buffer, static_cast<int>(buffer_len - 1)));

        buffer[converted] = L'\0';

        return converted;
    }

    static usize Win32FromWideChar(const wchar_t* input, char* buffer, usize buffer_len)
    {
        if (buffer_len == 0)
            return 0;

        usize converted = static_cast<usize>(
            WideCharToMultiByte(CP_UTF8, 0, input, -1, buffer, static_cast<int>(buffer_len), nullptr, nullptr));

        if (converted != 0)
            converted -= 1;

        return converted;
    }

    static usize Win32ToNativePath(StringView input, wchar_t* buffer, usize buffer_len)
    {
        usize converted = Win32ToWideChar(input, buffer, buffer_len);

        for (wchar_t* find = std::wcschr(buffer, L'/'); find; find = std::wcschr(find + 1, L'/'))
            *find = L'\\';

        if (std::wcsstr(buffer, L"..\\"))
        {
            converted = 0;

            buffer[converted] = L'\0';
        }

        return converted;
    }

    static usize Win32FromNativePath(const wchar_t* input, char* buffer, usize buffer_len)
    {
        usize converted = Win32FromWideChar(input, buffer, buffer_len);

        for (char* find = std::strchr(buffer, '\\'); find; find = std::strchr(find + 1, '\\'))
            *find = '/';

        return converted;
    }

    Win32FileStreamBase::Win32FileStreamBase(HANDLE handle)
        : handle_(handle)
    {}

    Win32FileStreamBase::~Win32FileStreamBase() = default;

    StreamPosition Win32FileStreamBase::Seek(i64 offset, SeekWhence whence)
    {
        LARGE_INTEGER distance;
        distance.QuadPart = offset;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        DWORD method = 0;

        switch (whence)
        {
            case SeekWhence::Set: method = FILE_BEGIN; break;
            case SeekWhence::Cur: method = FILE_CURRENT; break;
            case SeekWhence::End: method = FILE_END; break;
        }

        if (SetFilePointerEx(handle_, distance, &result, method))
            return result.QuadPart;

        return StreamPosition();
    }

    StreamPosition Win32FileStreamBase::Tell()
    {
        LARGE_INTEGER distance;
        distance.QuadPart = 0;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        if (SetFilePointerEx(handle_, distance, &result, FILE_CURRENT))
            return result.QuadPart;

        return StreamPosition();
    }

    StreamPosition Win32FileStreamBase::Size()
    {
        LARGE_INTEGER result;
        result.QuadPart = 0;

        if (GetFileSizeEx(handle_, &result))
            return result.QuadPart;

        return StreamPosition();
    }

    usize Win32FileStreamBase::Read(void* ptr, usize len)
    {
        DWORD result = 0;

        // TODO: Handle reads > 4gb
        IrAssert(len <= UINT32_MAX, "Cannot read more than 4gb at once");
        return ReadFile(handle_, ptr, static_cast<DWORD>(len), &result, nullptr) ? result : 0;
    }

    usize Win32FileStreamBase::Write(const void* ptr, usize len)
    {
        DWORD result = 0;

        // TODO: Handle writes > 4gb
        IrAssert(len <= UINT32_MAX, "Cannot write more than 4gb at once");
        return WriteFile(handle_, ptr, static_cast<DWORD>(len), &result, nullptr) ? result : 0;
    }

    usize Win32FileStreamBase::ReadBulk(void* ptr, usize len, u64 offset)
    {
        OVERLAPPED overlapped {};

        overlapped.Offset = offset & 0xFFFFFFFF;
        overlapped.OffsetHigh = offset >> 32;

        DWORD result = 0;

        // TODO: Handle reads > 4gb
        IrAssert(len <= UINT32_MAX, "Cannot read more than 4gb at once");
        return ReadFile(handle_, ptr, static_cast<DWORD>(len), &result, &overlapped) ? result : 0;
    }

    usize Win32FileStreamBase::WriteBulk(const void* ptr, usize len, u64 offset)
    {
        OVERLAPPED overlapped {};

        overlapped.Offset = offset & 0xFFFFFFFF;
        overlapped.OffsetHigh = offset >> 32;

        DWORD result = 0;

        // TODO: Handle writes > 4gb
        IrAssert(len <= UINT32_MAX, "Cannot write more than 4gb at once");
        return WriteFile(handle_, ptr, static_cast<DWORD>(len), &result, &overlapped) ? result : 0;
    }

    bool Win32FileStreamBase::Flush()
    {
        return FlushFileBuffers(handle_);
    }

    StreamPosition Win32FileStreamBase::SetSize(u64 length)
    {
        LARGE_INTEGER distance;
        distance.QuadPart = length;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        if (SetFilePointerEx(handle_, distance, &result, FILE_BEGIN) && (static_cast<u64>(result.QuadPart) == length) &&
            SetEndOfFile(handle_))
            return length;

        return StreamPosition();
    }

    bool Win32FileStreamBase::IsBulkSync()
    {
        return true;
    }

    bool Win32FileStreamBase::IsFullSync()
    {
        return true;
    }

    Win32FileStream::~Win32FileStream()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
            CloseHandle(handle_);
    }

    Win32TempFileStream::~Win32TempFileStream()
    {
        Win32TempFileCache::Instance.Close(handle_);
    }

    Win32FindFileHandle::Win32FindFileHandle(const wchar_t* path)
    {
        handle_ = FindFirstFileExW(path, FindExInfoBasic, &data_, FindExSearchNameMatch, nullptr, 0);
        valid_ = handle_ != INVALID_HANDLE_VALUE;

        // Skip "." and ".." directories
        for (; valid_; valid_ = FindNextFileW(handle_, &data_))
        {
            if ((data_.cFileName[0] != L'.') ||
                (data_.cFileName[1] != L'\0') && (data_.cFileName[1] != L'.' || data_.cFileName[2] != L'\0'))
                break;
        }
    }

    Win32FindFileHandle::~Win32FindFileHandle()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
            FindClose(handle_);
    }

    bool Win32FindFileHandle::Next(FolderEntry& entry)
    {
        entry.Reset();

        if (!valid_)
            return false;

        char buffer[MAX_PATH];

        usize converted = Win32FromNativePath(data_.cFileName, buffer, std::size(buffer));

        if (converted != 0)
            entry.Name = StringView(buffer, converted);
        else
            entry.Name = "**Invalid**"_sv;

        entry.IsFolder = data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        entry.Size = data_.nFileSizeLow | static_cast<u64>(data_.nFileSizeHigh) << 32;

        valid_ = FindNextFileW(handle_, &data_);

        return true;
    }

    bool Win32FindVolumeHandle::Next(FolderEntry& entry)
    {
        entry.Reset();

        while (index_ <= 'Z')
        {
            char path[4] {index_++, ':', '\\', '\0'};

            if (GetVolumeInformationA(path, nullptr, 0, nullptr, nullptr, nullptr, nullptr, 0))
            {
                entry.Name = StringView(path, 2);
                entry.IsFolder = true;

                return true;
            }
        }

        return false;
    }

    Win32TempFileCache::Win32TempFileCache()
    {
        InitializeCriticalSection(&lock_);

        IrAssert(
            GetTempPathW(static_cast<DWORD>(std::size(temp_path_)), temp_path_) != 0, "Could not get temp file path");
    }

    Win32TempFileCache::~Win32TempFileCache()
    {
        CloseAll();

        DeleteCriticalSection(&lock_);
    }

    HANDLE Win32TempFileCache::Open()
    {
        HANDLE result = INVALID_HANDLE_VALUE;

        EnterCriticalSection(&lock_);

        if (handle_count_ != 0)
        {
            result = handles_[--handle_count_];
        }

        LeaveCriticalSection(&lock_);

        if (result == INVALID_HANDLE_VALUE)
        {
            for (usize tries = 0; tries < 100; ++tries)
            {
                wchar_t wpath[MAX_PATH];

                if (GetTempFileNameW(temp_path_, L"Ir", 0, wpath) == 0)
                    return nullptr;

                result = CreateFileW(wpath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);

                if ((result != INVALID_HANDLE_VALUE) || (GetLastError() != ERROR_SHARING_VIOLATION))
                    break;
            }
        }

        return result;
    }

    void Win32TempFileCache::Close(HANDLE handle)
    {
        EnterCriticalSection(&lock_);

        if (handle_count_ < std::size(handles_) && (SetFilePointer(handle, 0, NULL, FILE_BEGIN) == 0) &&
            SetEndOfFile(handle))
        {
            handles_[handle_count_++] = handle;
        }
        else
        {
            CloseHandle(handle);
        }

        LeaveCriticalSection(&lock_);
    }

    void Win32TempFileCache::CloseAll()
    {
        EnterCriticalSection(&lock_);

        while (handle_count_ != 0)
        {
            CloseHandle(handles_[--handle_count_]);
        }

        LeaveCriticalSection(&lock_);
    }

    static inline Rc<Stream> Win32CreateFile(const wchar_t* path, DWORD desired_access, DWORD share_mode,
        DWORD create_disposition, DWORD flags_and_attributes = FILE_ATTRIBUTE_NORMAL)
    {
        HANDLE handle =
            CreateFileW(path, desired_access, share_mode, nullptr, create_disposition, flags_and_attributes, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
            return nullptr;

        return MakeUnique<Win32FileStream>(handle);
    }

    static inline bool Win32CreateFolder(const wchar_t* path)
    {
        return CreateDirectoryW(path, NULL) || (GetLastError() == ERROR_ALREADY_EXISTS);
    }

    static inline bool Win32DeleteFile(const wchar_t* path)
    {
        return DeleteFileW(path);
    }

    static inline bool Win32DeleteFolder(const wchar_t* path)
    {
        return RemoveDirectoryW(path);
    }

    static inline bool Win32FileExists(const wchar_t* path)
    {
        DWORD attribs = GetFileAttributesW(path);

        return (attribs != INVALID_FILE_ATTRIBUTES) && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    static inline bool Win32FolderExists(const wchar_t* path)
    {
        DWORD attribs = GetFileAttributesW(path);

        return (attribs != INVALID_FILE_ATTRIBUTES) && (attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool PlatformIoInit()
    {
        return true;
    }

    void PlatformIoShutdown()
    {
        Win32TempFileCache::Instance.CloseAll();
    }

    Rc<Stream> PlatformOpenFile(StringView path, bool read_only)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return nullptr;

        if (!Win32FileExists(wpath))
            return nullptr;

        return Win32CreateFile(wpath, read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
            read_only ? FILE_SHARE_READ : 0, OPEN_EXISTING);
    }

    Rc<Stream> PlatformCreateFile(StringView path, bool write_only, bool truncate)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return nullptr;

        return Win32CreateFile(wpath, write_only ? GENERIC_WRITE : (GENERIC_READ | GENERIC_WRITE), 0,
            truncate ? CREATE_ALWAYS : OPEN_ALWAYS);
    }

    Ptr<FindFileHandle> PlatformFindFiles(StringView path)
    {
        if (path.empty())
            return MakeUnique<Win32FindVolumeHandle>();

        wchar_t wpath[MAX_PATH + 2];

        usize converted = Win32ToNativePath(path, wpath, std::size(wpath) - 2);

        if (converted == 0)
            return nullptr;

        if (wpath[converted - 1] != L'\\')
            return nullptr;

        wpath[converted++] = L'*';
        wpath[converted++] = L'\0';

        return MakeUnique<Win32FindFileHandle>(wpath);
    }

    Rc<Stream> PlatformTempStream()
    {
        HANDLE handle = Win32TempFileCache::Instance.Open();

        if (handle == INVALID_HANDLE_VALUE)
            return nullptr;

        return MakeRc<Win32TempFileStream>(handle);
    }

    bool PlatformCreateFolder(StringView path, bool recursive)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return false;

        if (Win32FolderExists(wpath))
            return true;

        if (!recursive)
            return Win32CreateFolder(wpath);

        bool exists = true;

        for (wchar_t* j = std::wcschr(wpath, L'\\'); j; j = std::wcschr(j + 1, L'\\'))
        {
            *j = L'\0';

            if (exists)
                exists = Win32FolderExists(wpath);

            if (!exists && !Win32CreateFolder(wpath))
                return nullptr;

            *j = L'\\';
        }

        return true;
    }

    bool PlatformDeleteFile(StringView path)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return false;

        return !Win32FileExists(wpath) || Win32DeleteFile(wpath);
    }

    bool PlatformDeleteFolder(StringView path)
    {
        wchar_t wpath[MAX_PATH];

        usize converted = Win32ToNativePath(path, wpath, std::size(wpath));

        if (converted == 0)
            return false;

        return !Win32FolderExists(wpath) || Win32DeleteFolder(wpath);
    }

    bool PlatformFileExists(StringView path)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return false;

        return Win32FileExists(wpath);
    }

    bool PlatformFolderExists(StringView path)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return false;

        return Win32FolderExists(wpath);
    }

    String PlatformPathExpandEnvStrings(StringView path)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return String(path);

        wchar_t wpath_new[MAX_PATH];

        if (ExpandEnvironmentStringsW(wpath, wpath_new, static_cast<DWORD>(std::size(wpath))) == 0)
            return String(path);

        char buffer[MAX_PATH];

        usize converted = Win32FromNativePath(wpath_new, buffer, std::size(buffer));

        if (converted == 0)
            return String(path);

        return String(buffer, buffer + converted);
    }
} // namespace Iridium
