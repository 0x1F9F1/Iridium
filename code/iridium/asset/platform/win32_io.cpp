#include "asset/filewatcher.h"
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

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;
        usize Write(const void* ptr, usize len) override;

        usize ReadBulk(void* ptr, usize len, u64 offset) override;
        usize WriteBulk(const void* ptr, usize len, u64 offset) override;

        bool Flush() override;

        i64 SetSize(u64 length) override;

        bool IsBulkSync() const override;
        bool IsFullSync() const override;

    protected:
        HANDLE handle_ {INVALID_HANDLE_VALUE};
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
        HANDLE handle_ {INVALID_HANDLE_VALUE};
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

        void InitTempPath();

        static Win32TempFileCache Instance;

    private:
        CRITICAL_SECTION lock_ {};
        wchar_t temp_path_[MAX_PATH + 1] {};
        usize handle_count_ {0};
        HANDLE handles_[16] {};
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

    static usize Win32FromWideChar(const wchar_t* input, usize input_len, char* buffer, usize buffer_len)
    {
        if (buffer_len == 0)
            return 0;

        usize converted = static_cast<usize>(WideCharToMultiByte(
            CP_UTF8, 0, input, static_cast<int>(input_len), buffer, static_cast<int>(buffer_len), nullptr, nullptr));

        if (input_len == SIZE_MAX)
        {
            if (converted != 0)
                converted -= 1;
        }
        else
        {
            if (converted == buffer_len)
                converted -= 1;

            buffer[converted] = '\0';
        }

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

    static usize Win32FromNativePath(const wchar_t* input, usize input_len, char* buffer, usize buffer_len)
    {
        usize converted = Win32FromWideChar(input, input_len, buffer, buffer_len);

        for (char* find = std::strchr(buffer, '\\'); find; find = std::strchr(find + 1, '\\'))
            *find = '/';

        return converted;
    }

    Win32FileStreamBase::Win32FileStreamBase(HANDLE handle)
        : handle_(handle)
    {}

    Win32FileStreamBase::~Win32FileStreamBase() = default;

    i64 Win32FileStreamBase::Seek(i64 offset, SeekWhence whence)
    {
        LARGE_INTEGER distance;
        distance.QuadPart = offset;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        DWORD method = FILE_BEGIN;

        switch (whence)
        {
            case SeekWhence::Set: method = FILE_BEGIN; break;
            case SeekWhence::Cur: method = FILE_CURRENT; break;
            case SeekWhence::End: method = FILE_END; break;
        }

        return SetFilePointerEx(handle_, distance, &result, method) ? result.QuadPart : -1;
    }

    i64 Win32FileStreamBase::Tell()
    {
        LARGE_INTEGER distance;
        distance.QuadPart = 0;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        return SetFilePointerEx(handle_, distance, &result, FILE_CURRENT) ? result.QuadPart : -1;
    }

    i64 Win32FileStreamBase::Size()
    {
        LARGE_INTEGER result;
        result.QuadPart = 0;

        return GetFileSizeEx(handle_, &result) ? result.QuadPart : -1;
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

    i64 Win32FileStreamBase::SetSize(u64 length)
    {
        LARGE_INTEGER distance;
        distance.QuadPart = length;

        LARGE_INTEGER result;
        result.QuadPart = 0;

        return (SetFilePointerEx(handle_, distance, &result, FILE_BEGIN) && (result.QuadPart == i64(length)) &&
                   SetEndOfFile(handle_))
            ? length
            : -1;
    }

    bool Win32FileStreamBase::IsBulkSync() const
    {
        return true;
    }

    bool Win32FileStreamBase::IsFullSync() const
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
                ((data_.cFileName[1] != L'\0') && (data_.cFileName[1] != L'.' || data_.cFileName[2] != L'\0')))
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

        usize converted = Win32FromNativePath(data_.cFileName, SIZE_MAX, buffer, std::size(buffer));

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
    }

    Win32TempFileCache::~Win32TempFileCache()
    {
        CloseAll();

        DeleteCriticalSection(&lock_);

        if (temp_path_[0])
            RemoveDirectoryW(temp_path_);
    }

    HANDLE Win32TempFileCache::Open()
    {
        EnterCriticalSection(&lock_);

        if (temp_path_[0] == L'\0')
            InitTempPath();

        HANDLE result = (handle_count_ != 0) ? handles_[--handle_count_] : INVALID_HANDLE_VALUE;

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
        if (handle == INVALID_HANDLE_VALUE)
            return;

        EnterCriticalSection(&lock_);

        if ((handle_count_ < std::size(handles_)) && (SetFilePointer(handle, 0, NULL, FILE_BEGIN) == 0) &&
            SetEndOfFile(handle))
        {
            handles_[handle_count_++] = handle;
            handle = INVALID_HANDLE_VALUE;
        }

        LeaveCriticalSection(&lock_);

        if (handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
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

    IR_NOINLINE void Win32TempFileCache::InitTempPath()
    {
        usize written = GetTempPathW(static_cast<DWORD>(std::size(temp_path_)), temp_path_);

        IrAssert(written != 0, "Could not get temp file path");
        IrAssert(written + 11 < std::size(temp_path_), "Temp file path too long");

        temp_path_[written++] = L'I';
        temp_path_[written++] = L'r';

        DWORD pid = GetCurrentProcessId();

        for (usize i = 8; i--; pid >>= 4)
            temp_path_[written + i] = L"0123456789ABCDEF"[pid & 0xF];

        written += 8;

        temp_path_[written++] = L'\\';
        temp_path_[written] = L'\0';

        IrAssert(CreateDirectoryW(temp_path_, NULL) || (GetLastError() == ERROR_ALREADY_EXISTS),
            "Failed to create temp directory");
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

    class Win32FileSystemWatcher final : public FileSystemWatcher
    {
    public:
        Win32FileSystemWatcher(HANDLE handle, NotifyFilters filter)
            : handle_(handle)
        {
            if (filter & NotifyFilter::FileName)
                filter_ |= FILE_NOTIFY_CHANGE_FILE_NAME;

            if (filter & NotifyFilter::FolderName)
                filter_ |= FILE_NOTIFY_CHANGE_DIR_NAME;

            if (filter & NotifyFilter::Attributes)
                filter_ |= FILE_NOTIFY_CHANGE_ATTRIBUTES;

            if (filter & NotifyFilter::Size)
                filter_ |= FILE_NOTIFY_CHANGE_SIZE;

            if (filter & NotifyFilter::LastWrite)
                filter_ |= FILE_NOTIFY_CHANGE_LAST_WRITE;

            if (filter & NotifyFilter::LastAccess)
                filter_ |= FILE_NOTIFY_CHANGE_LAST_ACCESS;

            if (filter & NotifyFilter::CreationTime)
                filter_ |= FILE_NOTIFY_CHANGE_CREATION;

            if (handle_ != INVALID_HANDLE_VALUE)
            {
                event_.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

                RefreshWatch();
            }
        }

        ~Win32FileSystemWatcher() override
        {
            if (handle_ != INVALID_HANDLE_VALUE)
            {
                CancelIoEx(handle_, &event_);

                DWORD bytes_read = 0;
                GetOverlappedResult(handle_, &event_, &bytes_read, TRUE);

                CloseHandle(event_.hEvent);
                CloseHandle(handle_);
            }
        }

        bool Poll(FileSystemListener& listener, u32 timeout) override
        {
            DWORD bytes_read = 0;

            if (!GetOverlappedResultEx(handle_, &event_, &bytes_read, timeout, FALSE))
                return false;

            if (bytes_read != 0)
                HandleEvents(listener);

            RefreshWatch();

            return true;
        }

    private:
        inline bool RefreshWatch()
        {
            return ReadDirectoryChangesW(handle_, buffer_, sizeof(buffer_), FALSE, filter_, NULL, &event_, NULL);
        }

        inline void HandleEvents(FileSystemListener& listener)
        {
            FILE_NOTIFY_INFORMATION* info = nullptr;
            FILE_NOTIFY_INFORMATION* next = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer_);

            while (info != next)
            {
                info = next;
                next = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<unsigned char*>(info) + info->NextEntryOffset);

                if (info->Action == FILE_ACTION_RENAMED_NEW_NAME)
                    continue;

                char file_name[MAX_PATH];

                Win32FromNativePath(
                    info->FileName, info->FileNameLength / sizeof(info->FileName[0]), file_name, std::size(file_name));

                switch (info->Action)
                {
                    case FILE_ACTION_ADDED: {
                        listener.OnAdded(file_name);
                        break;
                    }

                    case FILE_ACTION_REMOVED: {
                        listener.OnRemoved(file_name);
                        break;
                    }

                    case FILE_ACTION_MODIFIED: {
                        listener.OnModified(file_name);
                        break;
                    }

                    case FILE_ACTION_RENAMED_OLD_NAME: {
                        char new_file_name[MAX_PATH];

                        Win32FromNativePath(next->FileName, next->FileNameLength / sizeof(next->FileName[0]),
                            new_file_name, std::size(new_file_name));

                        listener.OnRenamed(file_name, new_file_name);

                        break;
                    }
                }
            }
        }

        HANDLE handle_ {INVALID_HANDLE_VALUE};
        DWORD filter_ {};
        OVERLAPPED event_ {};
        alignas(DWORD) BYTE buffer_[32 * 1024];
    };

    static inline Ptr<FileSystemWatcher> Win32CreateFileSystemWatcher(const wchar_t* path, NotifyFilters filter)
    {
        // CreateFileW, ReadDirectoryChangesW, CloseHandle

        HANDLE handle = CreateFileW(path, GENERIC_READ | FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

        if (handle == INVALID_HANDLE_VALUE)
            return nullptr;

        return MakeUnique<Win32FileSystemWatcher>(handle, filter);
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

    Ptr<FileSystemWatcher> PlatformCreateFileSystemWatcher(StringView path, NotifyFilters filter)
    {
        wchar_t wpath[MAX_PATH];

        if (Win32ToNativePath(path, wpath, std::size(wpath)) == 0)
            return nullptr;

        return Win32CreateFileSystemWatcher(wpath, filter);
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
                return false;

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

        DWORD length = ExpandEnvironmentStringsW(wpath, wpath_new, static_cast<DWORD>(std::size(wpath)));

        if (length == 0)
            return String(path);

        char buffer[MAX_PATH];

        usize converted = Win32FromNativePath(wpath_new, length, buffer, std::size(buffer));

        if (converted == 0)
            return String(path);

        return String(buffer, buffer + converted);
    }
} // namespace Iridium
