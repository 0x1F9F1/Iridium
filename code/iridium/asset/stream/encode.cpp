#include "encode.h"

#include "asset/transform.h"

namespace Iridium
{
    EncodeStream::EncodeStream(Rc<Stream> output, Ptr<BinaryTransform> transform, usize buffer_size)
        : output_(std::move(output))
        , transform_(std::move(transform))
        , buffer_size_(buffer_size)
    {
        buffer_.reset(new u8[buffer_size_]);
    }

    EncodeStream::~EncodeStream()
    {
        Flush();
    }

    StreamPosition EncodeStream::Tell()
    {
        return size_;
    }

    StreamPosition EncodeStream::Size()
    {
        return size_;
    }

    usize EncodeStream::Write(const void* ptr, usize len)
    {
        transform_->NextIn = static_cast<const u8*>(ptr);
        transform_->AvailIn = len;

        while (transform_->AvailIn && !transform_->Finished)
        {
            transform_->NextOut = &buffer_[0];
            transform_->AvailOut = buffer_size_;

            if (!transform_->Update())
            {
                break;
            }

            usize written = buffer_size_ - transform_->AvailOut;

            if (written != 0)
            {
                size_ += output_->Write(&buffer_[0], written);
            }
        }

        return len - transform_->AvailIn;
    }

    bool EncodeStream::Flush()
    {
        transform_->NextIn = nullptr;
        transform_->AvailIn = 0;
        transform_->Finished = true;

        while (true)
        {
            transform_->NextOut = &buffer_[0];
            transform_->AvailOut = buffer_size_;

            if (!transform_->Update())
            {
                return false;
            }

            usize written = buffer_size_ - transform_->AvailOut;

            if (written == 0)
                break;

            size_ += output_->Write(&buffer_[0], written);
        }

        return true;
    }
} // namespace Iridium
