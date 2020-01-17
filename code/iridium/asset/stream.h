#pragma once

namespace Iridium
{
    enum class SeekWhence : u8
    {
        Set,
        Cur,
        End
    };

    class Stream : public AtomicRefCounted
    {
    public:
        // Sets the current file position
        // Returns the new file position, or -1 on error
        virtual i64 Seek(i64 position, SeekWhence whence);

        // Retrieves the current file position
        // Returns the current file position, or -1 on error
        virtual i64 Tell();

        // Retrieves the file size
        // May modify the current file position
        // Returns the file size, or -1 on error
        virtual i64 Size();

        // Reads up to len bytes from the current file position
        // Increments the file position by the number of bytes read
        // Returns the number of bytes read
        virtual usize Read(void* ptr, usize len);

        // Writes up to len bytes to the current file position
        // Increments the file position by the number of bytes written
        // Returns the number of bytes written
        virtual usize Write(const void* ptr, usize len);

        // Reads up to len bytes from the specified file position
        // May modify the current file position
        // Returns the number of bytes read
        virtual usize ReadBulk(void* ptr, usize len, i64 offset);

        // Writes up to len bytes to the specified file position
        // May modify the current file position
        // Returns the number of bytes written
        virtual usize WriteBulk(const void* ptr, usize len, i64 offset);

        // Flushes any buffers, causing buffered data to be written to the underlying file
        // May modify the current file position
        // Returns true if successful
        virtual bool Flush();

        // Sets the file size
        // May modify the current file position
        // Returns the new file size, or -1 on error
        virtual i64 SetSize(i64 length);

        // Copies current stream contents to output
        // Returns number of bytes copied
        virtual usize CopyTo(Stream& output);

        // Retreives the underlying file handle usable for bulk operations
        // Returns the bulk file handle, and adjusts offset as required
        virtual Rc<Stream> GetBulkStream(i64& offset, i64 size);

        // TODO: Use flags instead of functions for these?
        // IsOpen, IsEndOfFile, HasError
        // CanRead, CanWrite, CanSeek
        // IsBulkSync, IsFullSync, IsBuffered
        // IsTemporary, IsRemote, IsRandomAccess
        // IsBuffered, IsCompressed, IsEncrypted

        virtual bool IsBulkSync();
        virtual bool IsFullSync();

        // virtual bool IsCompressed()
        // virtual bool IsEncrypted()
        // virtual bool IsRemote()
        // virtual bool IsBuffered();

        // static Rc<Stream> Buffered(Rc<Stream> stream);

        static Rc<Stream> BulkSync(Rc<Stream> stream);
        static Rc<Stream> FullSync(Rc<Stream> stream);

        static Rc<Stream> Temp();

        String ReadText();
        Vec<u8> ReadBytes();
    };
} // namespace Iridium
