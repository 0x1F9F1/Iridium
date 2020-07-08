#include "decode.h"

#include "asset/transform.h"

namespace Iridium
{
    DecodeStream::DecodeStream(Rc<Stream> handle, Ptr<BinaryTransform> transform, u64 size, usize buffer_size)
        : input_(std::move(handle))
        , transform_(std::move(transform))
        , size_(size)
        , buffer_size_(buffer_size)
    {
        buffer_.reset(new u8[buffer_size_]);
    }

    i64 DecodeStream::Seek(i64 offset, SeekWhence whence)
    {
        switch (whence)
        {
            case SeekWhence::Set: break;
            case SeekWhence::Cur: offset += current_; break;
            case SeekWhence::End: offset += size_; break;
        }

        offset = std::clamp<i64>(offset, 0, size_);

        if (offset == current_)
            return current_;

        if (offset == i64(size_))
        {
            current_ = size_;

            return current_;
        }

        if (offset < current_)
        {
            if (!input_->TrySeek(0))
                return -1;

            if (!transform_->Reset())
                return -1;

            transform_->NextIn = nullptr;
            transform_->AvailIn = 0;

            current_ = 0;
        }

        if (current_ != offset)
        {
            // IrLogInfo("DecodeStream::Seek - Discarding {} bytes", offset - current_);

            u8 buffer[8192];

            while (current_ < offset)
            {
                usize len = static_cast<usize>(std::min<i64>(offset - current_, sizeof(buffer)));

                if (Read(buffer, len) != len)
                    break;
            }
        }

        if (current_ != offset)
        {
            current_ = -1;
        }

        return current_;
    }

    i64 DecodeStream::Tell()
    {
        return current_;
    }

    i64 DecodeStream::Size()
    {
        return size_;
    }

    usize DecodeStream::Read(void* ptr, usize len)
    {
        if (current_ < 0 || current_ >= i64(size_))
            return 0;

        len = static_cast<usize>(std::min<i64>(size_ - current_, len));

        transform_->NextOut = static_cast<u8*>(ptr);
        transform_->AvailOut = len;

        while (transform_->AvailOut && !transform_->Finished)
        {
            if (!transform_->AvailIn)
            {
                usize raw_len = input_->Read(&buffer_[0], buffer_size_);

                transform_->NextIn = &buffer_[0];
                transform_->AvailIn = raw_len;
            }

            if (!transform_->Update())
            {
                // IrLogError("DecodeStream::Read - Transform.Update Failed");

                break;
            }
        }

        len -= transform_->AvailOut;

        transform_->NextOut = nullptr;
        transform_->AvailOut = 0;

        current_ += len;

        return len;
    }
} // namespace Iridium
