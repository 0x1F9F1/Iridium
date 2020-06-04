#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class BufferedStream final : public Stream
    {
    public:
        BufferedStream(Rc<Stream> handle);
        ~BufferedStream() override;

        StreamPosition Seek(i64 offset, SeekWhence whence) override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;
        usize Write(const void* ptr, usize len) override;

        usize ReadBulk(void* ptr, usize len, u64 offset) override;
        usize WriteBulk(const void* ptr, usize len, u64 offset) override;

        bool Flush() override;

        bool FlushBuffer();

        /*
        template <typename T>
        [[nodiscard]] bool Get(T& value);

        template <typename T>
        [[nodiscard]] bool Get(T* values, usize count);

        template <typename T>
        T Get();
        */

        i32 GetCh();
        i32 UnGetCh(i32 ch);

        i32 PutCh(i32 ch);

        bool GetLine(String& output, char delim = '\n');

        void PutString(StringView str);

        /*
        static Ptr<BufferedStream> Open(StringView path, bool read_only = true);
        static Ptr<BufferedStream> Create(StringView path, bool write_only = true, bool truncate = true);

        static Ptr<BufferedStream> Open(std::initializer_list<StringView> parts, StringView ext, bool read_only = true);

        static Ptr<BufferedStream> Create(
            std::initializer_list<StringView> parts, StringView ext, bool write_only = true, bool truncate = true);
        */

        VIRTUAL_META_DECLARE;

    private:
        StreamPosition RawTell() const;
        StreamPosition RawSize() const;
        StreamPosition RawSeek(i64 offset, SeekWhence whence);

        usize RawRead(void* ptr, usize len);
        usize RawWrite(const void* ptr, usize len);

        bool SeekBuffer(i64 offset, SeekWhence whence);

        bool RawFlush();

        bool FlushReads();
        bool FlushWrites();

        // Underlying file handle
        Rc<Stream> handle_;

        // Buffer for more efficient reads/writes
        Ptr<u8[]> buffer_ {nullptr};

        // Cached file position, (position_ + buffer_read_ == RawTell())
        StreamPosition position_ {};

        // Maximum capacity of the buffer
        u32 buffer_capacity_ {0};

        // Number of bytes read from or written into the buffer
        u32 buffer_head_ {0};

        // Number of bytes read from the file for buffering
        u32 buffer_read_ {0};
    };

    inline StreamPosition BufferedStream::Tell()
    {
        return position_ + buffer_head_;
    }

    /*
    template <typename T>
    inline bool BufferedStream::Get(T& value)
    {
        return Read(&value, sizeof(value)) == sizeof(value);
    }

    template <typename T>
    inline bool BufferedStream::Get(T* values, usize count)
    {
        return Read(values, count * sizeof(*values)) == count * sizeof(*values);
    }

    template <typename T>
    inline T BufferedStream::Get()
    {
        T value;

        if (Read(&value, sizeof(value)) == sizeof(value))
        {
            return value;
        }

        return {};
    }
    */

    inline i32 BufferedStream::GetCh()
    {
        if (buffer_head_ < buffer_read_)
        {
            return buffer_[buffer_head_++];
        }

        u8 result = 0;

        if (Read(&result, 1))
        {
            return result;
        }

        return -1;
    }

    inline i32 BufferedStream::UnGetCh(i32 ch)
    {
        if (buffer_read_ != 0 && buffer_head_ != 0)
        {
            buffer_[--buffer_head_] = static_cast<u8>(ch);

            return ch;
        }

        return -1;
    }

    inline i32 BufferedStream::PutCh(i32 ch)
    {
        if (buffer_read_ == 0 && buffer_head_ < buffer_capacity_)
        {
            buffer_[buffer_head_++] = static_cast<u8>(ch);

            return ch;
        }

        u8 result = static_cast<u8>(ch);

        if (Write(&result, 1))
        {
            return ch;
        }

        return -1;
    }

    inline void BufferedStream::PutString(StringView str)
    {
        Write(str.data(), str.size());
    }
} // namespace Iridium
