#include "stream.h"

#include "core/meta/metadefine.h"
#include "platform_io.h"
#include "stream/buffered.h"
#include "stream/sync.h"

namespace Iridium
{
    /*
        int close(int fd);

        int fclose(std::FILE* stream);

        BOOL CloseHandle(HANDLE hObject);
    */

    /*
        off_t lseek(int fd, off_t offset, int whence);
        off64_t lseek64(int fd, off64_t offset, int whence);

        int fseek(std::FILE* stream, long offset, int origin);

        DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
        BOOL SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
    */
    i64 Stream::Seek(i64, SeekWhence)
    {
        return -1;
    }

    /*
        lseek(fd, 0, SEEK_CUR);

        long ftell(std::FILE* stream);

        SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    */
    i64 Stream::Tell()
    {
        return Seek(0, SeekWhence::Cur);
    }

    /*
        int fstat(int fd, struct stat *buf);

        lseek(fd, 0, SEEK_END);

        BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    */
    i64 Stream::Size()
    {
        i64 const here = Tell();
        i64 const size = Seek(0, SeekWhence::End);

        if (here >= 0)
            Seek(here, SeekWhence::Set);

        return size;
    }

    /*
        ssize_t read(int fd, void* buf, size_t count);

        std::size_t fread(void* buffer, std::size_t size, std::size_t count, std::FILE* stream);

        BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    */
    usize Stream::Read(void*, usize)
    {
        return 0;
    }

    /*
        ssize_t write(int fd, const void* buf, size_t count);

        std::size_t fwrite(const void* buffer, std::size_t size, std::size_t count, std::FILE* stream);

        BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    */
    usize Stream::Write(const void*, usize)
    {
        return 0;
    }

    /*
        ssize_t pread(int fd, void* buf, size_t count, off_t offset);

        BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    */
    usize Stream::ReadBulk(void* ptr, usize len, u64 offset)
    {
        return (Seek(offset, SeekWhence::Set) == static_cast<i64>(offset)) ? Read(ptr, len) : 0;
    }

    /*
        ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);

        BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    */
    usize Stream::WriteBulk(const void* ptr, usize len, u64 offset)
    {
        return (Seek(offset, SeekWhence::Set) == static_cast<i64>(offset)) ? Write(ptr, len) : 0;
    }

    /*
        int fsync(int fd);
        int fdatasync(int fd);

        int fflush(std::FILE* stream);

        BOOL FlushFileBuffers(HANDLE hFile);
    */
    bool Stream::Flush()
    {
        return true;
    }

    /*
        int ftruncate(int fd, off_t length);

        BOOL SetEndOfFile(HANDLE hFile);
    */
    i64 Stream::SetSize(u64)
    {
        return -1;
    }

    u64 Stream::CopyTo(Stream& output)
    {
        u64 total = 0;

        u8 buffer[32768];

        while (true)
        {
            usize len = Read(buffer, sizeof(buffer));

            if (len == 0)
                break;

            len = output.Write(buffer, len);

            if (len == 0)
                break;

            total += len;
        }

        return total;
    }

    Rc<Stream> Stream::GetBulkStream(u64&, u64)
    {
        return nullptr;
    }

    bool Stream::IsBulkSync() const
    {
        return IsFullSync();
    }

    bool Stream::IsFullSync() const
    {
        return false;
    }

    Rc<BufferedStream> Stream::Buffered(Rc<Stream> stream)
    {
        if (stream->IsA<BufferedStream>())
            return StaticCast<BufferedStream>(std::move(stream));

        return MakeRc<BufferedStream>(std::move(stream));
    }

    Rc<Stream> Stream::BulkSync(Rc<Stream> stream)
    {
        if (stream->IsBulkSync())
            return stream;

        return MakeRc<SyncStream>(std::move(stream));
    }

    Rc<Stream> Stream::FullSync(Rc<Stream> stream)
    {
        if (stream->IsFullSync())
            return stream;

        return MakeRc<SyncStream>(std::move(stream));
    }

    Rc<Stream> Stream::Temp()
    {
        return PlatformTempStream();
    }

    VIRTUAL_META_DEFINE_CHILD("Stream", Stream, AtomicRefCounted)
    {}
} // namespace Iridium
