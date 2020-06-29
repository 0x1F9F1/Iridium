#include "buffered.h"

#include "core/meta/metadefine.h"

namespace Iridium
{
    BufferedStream::BufferedStream(Rc<Stream> handle, usize capacity)
        : handle_(std::move(handle))
    {
        buffer_capacity_ = static_cast<u32>(capacity);
        buffer_.reset(new u8[buffer_capacity_]);
        position_ = handle_->Tell();
    }

    BufferedStream::~BufferedStream()
    {
        FlushWrites();
    }

    StreamPosition BufferedStream::Size()
    {
        return FlushWrites() ? handle_->Size() : StreamPosition();
    }

    StreamPosition BufferedStream::Seek(i64 offset, SeekWhence whence)
    {
        if (buffer_read_ != 0)
        {
            i64 rel_offset = offset;

            switch (whence) // Calculate the offset relative to the start of the buffer
            {
                case SeekWhence::End: rel_offset += handle_->Size().get(); [[fallthrough]];
                case SeekWhence::Set: rel_offset -= position_.get(); break;
                case SeekWhence::Cur: rel_offset += buffer_head_; break;
            }

            if (rel_offset >= 0 && rel_offset <= buffer_read_)
            {
                buffer_head_ = static_cast<u32>(rel_offset);

                return position_ + static_cast<u64>(buffer_head_);
            }

            if (whence == SeekWhence::Cur)
            {
                offset -= buffer_read_ - buffer_head_;
            }
        }
        else if (!FlushWrites())
        {
            return StreamPosition();
        }

        position_ = handle_->Seek(offset, whence);

        buffer_head_ = 0;
        buffer_read_ = 0;

        return position_;
    }

    usize BufferedStream::Read(void* ptr, usize len)
    {
        if (!position_.valid() || !FlushWrites())
        {
            return 0;
        }

        usize total = 0;

        if (usize const buffered = buffer_read_ - buffer_head_; len > buffered)
        {
            if (buffered != 0)
            {
                std::memcpy(ptr, &buffer_[buffer_head_], buffered);
                buffer_head_ = buffer_read_;

                ptr = static_cast<u8*>(ptr) + buffered;
                len -= buffered;
                total += buffered;
            }

            position_ += static_cast<u64>(buffer_head_);

            if (len >= buffer_capacity_)
            {
                usize const raw_read = handle_->Read(ptr, len);

                buffer_head_ = 0;
                buffer_read_ = 0;

                position_ += u64(raw_read);
                total += raw_read;

                return total;
            }

            usize const raw_read = handle_->Read(&buffer_[0], buffer_capacity_);

            buffer_head_ = 0;
            buffer_read_ = static_cast<u32>(raw_read);

            if (len > raw_read)
                len = raw_read;
        }

        std::memcpy(ptr, &buffer_[buffer_head_], len);
        buffer_head_ += static_cast<u32>(len);
        total += len;

        return total;
    }

    usize BufferedStream::Write(const void* ptr, usize len)
    {
        if (!position_.valid() || !FlushReads())
        {
            return 0;
        }

        if (len < buffer_capacity_)
        {
            usize total = 0;

            usize const available = buffer_capacity_ - buffer_head_;

            if (len >= available)
            {
                std::memcpy(&buffer_[buffer_head_], ptr, available);

                ptr = static_cast<const u8*>(ptr) + available;
                len -= available;
                total += available;

                usize const written = handle_->Write(&buffer_[0], buffer_capacity_);

                position_ += u64(written);
                buffer_head_ = 0;

                if (written != buffer_capacity_)
                {
                    return available;
                }
            }

            std::memcpy(&buffer_[buffer_head_], ptr, len);

            buffer_head_ += static_cast<u32>(len);

            total += len;

            return total;
        }
        else if (FlushWrites())
        {
            usize const written = handle_->Write(ptr, len);

            position_ += u64(written);

            return written;
        }
        else
        {
            return 0;
        }
    }

    usize BufferedStream::ReadBulk(void* ptr, usize len, u64 offset)
    {
        if (!FlushWrites())
        {
            return 0;
        }

        position_ = StreamPosition();

        return handle_->ReadBulk(ptr, len, offset);
    }

    usize BufferedStream::WriteBulk(const void* ptr, usize len, u64 offset)
    {
        if (!FlushWrites())
        {
            return 0;
        }

        position_ = StreamPosition();

        return handle_->WriteBulk(ptr, len, offset);
    }

    bool BufferedStream::GetLine(String& output, char delim)
    {
        usize total = 0;

        while (true)
        {
            i32 const cur = GetCh();

            if (cur == -1)
            {
                if (total == 0)
                    return false;

                break;
            }

            if (cur == delim)
                break;

            output.push_back(static_cast<char>(cur));

            ++total;
        }

        if (total && delim == '\n' && output.back() == '\r')
        {
            output.pop_back();
        }

        return total;
    }

    bool BufferedStream::FlushBuffer()
    {
        return FlushReads() && FlushWrites();
    }

    bool BufferedStream::Flush()
    {
        return FlushBuffer() && handle_->Flush();
    }

    IR_FORCEINLINE bool BufferedStream::FlushReads()
    {
        if (buffer_read_ != 0 && buffer_read_ != buffer_head_ && position_.valid())
        {
            position_ = handle_->Seek(position_.get() + buffer_head_, SeekWhence::Set);

            buffer_head_ = 0;
            buffer_read_ = 0;
        }

        return position_.valid();
    }

    IR_FORCEINLINE bool BufferedStream::FlushWrites()
    {
        if (buffer_head_ <= buffer_read_)
        {
            return true;
        }

        usize const written = handle_->Write(&buffer_[0], buffer_head_);

        bool success = written == buffer_head_;

        position_ += u64(written);
        buffer_head_ = 0;

        return success;
    }

    VIRTUAL_META_DEFINE_CHILD("BufferedStream", BufferedStream, Stream)
    {}
} // namespace Iridium
