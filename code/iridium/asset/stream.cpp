#include "stream.h"

#include "stream/sync.h"

#include "platform_io.h"

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
    StreamPosition Stream::Seek(i64, SeekWhence)
    {
        return StreamPosition();
    }

    /*
        lseek(fd, 0, SEEK_CUR);

        long ftell(std::FILE* stream);

        SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    */
    StreamPosition Stream::Tell()
    {
        return Seek(0, SeekWhence::Cur);
    }

    /*
        int fstat(int fd, struct stat *buf);

        lseek(fd, 0, SEEK_END);

        BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    */
    StreamPosition Stream::Size()
    {
        StreamPosition const here = Tell();
        StreamPosition const size = Seek(0, SeekWhence::End);

        if (here.valid())
            Seek(here.get(), SeekWhence::Set);

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
        return (Seek(offset, SeekWhence::Set) == offset) ? Read(ptr, len) : 0;
    }

    /*
        ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);

        BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    */
    usize Stream::WriteBulk(const void* ptr, usize len, u64 offset)
    {
        return (Seek(offset, SeekWhence::Set) == offset) ? Write(ptr, len) : 0;
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
    StreamPosition Stream::SetSize(u64)
    {
        return StreamPosition();
    }

    usize Stream::CopyTo(Stream& output)
    {
        usize total = 0;

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

    bool Stream::IsBulkSync()
    {
        return IsFullSync();
    }

    bool Stream::IsFullSync()
    {
        return false;
    }

    Rc<Stream> Stream::BulkSync(Rc<Stream> stream)
    {
        if (stream->IsBulkSync())
            return std::move(stream);

        return MakeRc<SyncStream>(std::move(stream));
    }

    Rc<Stream> Stream::FullSync(Rc<Stream> stream)
    {
        if (stream->IsFullSync())
            return std::move(stream);

        return MakeRc<SyncStream>(std::move(stream));
    }

    Rc<Stream> Stream::Temp()
    {
        return PlatformTempStream();
    }

    String Stream::ReadText()
    {
        u64 const here = Tell().get(0);
        u64 const size = Size().get(0) - here;

        if ((here >= size) || ((size - here) > SIZE_MAX))
            return {};

        usize const maximum = static_cast<usize>(size - here);

        String result(maximum, '\0');

        usize total = 0;

        while (total < maximum)
        {
            usize const bytes_read = Read(&result[total], maximum - total);

            if (bytes_read == 0)
                break;

            total += bytes_read;
        }

        result.resize(total);

        return result;
    }

    Vec<u8> Stream::ReadBytes()
    {
        u64 const here = Tell().get(0);
        u64 const size = Size().get(0) - here;

        if ((here >= size) || ((size - here) > SIZE_MAX))
            return {};

        usize const maximum = static_cast<usize>(size - here);

        Vec<u8> result(maximum);

        usize total = 0;

        while (total < maximum)
        {
            usize const bytes_read = Read(&result[total], maximum - total);

            if (bytes_read == 0)
                break;

            total += bytes_read;
        }

        result.resize(total);

        return result;
    }
} // namespace Iridium
