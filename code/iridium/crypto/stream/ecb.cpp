#include "ecb.h"

#include "crypto/cipher.h"

namespace Iridium
{
    EcbCipherStream::EcbCipherStream(Rc<Stream> input, Ptr<Cipher> cipher)
        : input_(std::move(input))
        , cipher_(std::move(cipher))
        , block_size_(cipher_->GetBlockSize())
    {
        IrAssert(block_size_ <= sizeof(buffer_), "Block Size Too Large");
        IrAssert((block_size_ & (block_size_ - 1)) == 0, "Block Size Not Power Of 2");
    }

    EcbCipherStream::~EcbCipherStream() = default;

    StreamPosition EcbCipherStream::Seek(i64 offset, SeekWhence whence)
    {
        StreamPosition result = input_->Seek(offset, whence);

        buffered_ = 0;

        if (result.valid())
            padding_ = result.get() & (block_size_ - 1);

        return result;
    }

    StreamPosition EcbCipherStream::Tell()
    {
        return input_->Tell().get() - buffered_;
    }

    StreamPosition EcbCipherStream::Size()
    {
        return input_->Size();
    }

    usize EcbCipherStream::Read(void* ptr, usize len)
    {
        if (len == 0)
            return 0;

        usize total = 0;

        if (usize read = ReadBuffered(ptr, len))
        {
            ptr = static_cast<u8*>(ptr) + read;
            len -= read;
            total += read;

            if (len == 0)
                return total;
        }

        IrDebugAssert(buffered_ == 0, "");

        if (padding_)
        {
            if (input_->Seek(-i64(padding_), SeekWhence::Cur) < 0)
                return total;

            RefillBuffer();

            if (buffered_ <= padding_)
            {
                padding_ = buffered_;
                buffered_ = 0;

                return total;
            }

            buffered_ = padding_;
            padding_ = 0;

            if (usize read = ReadBuffered(ptr, len))
            {
                ptr = static_cast<u8*>(ptr) + read;
                len -= read;
                total += read;

                if (len == 0)
                    return total;
            }
        }

        usize const block_mask = block_size_ - 1;

        if (usize body = len & ~block_mask)
        {
            usize read = input_->Read(ptr, body);

            cipher_->Update(ptr, read & ~block_mask);

            ptr = static_cast<u8*>(ptr) + read;
            len -= read;
            total += read;

            if (len == 0)
                return total;

            if (read != body)
            {
                padding_ = read & block_mask;
                buffered_ = 0;

                return total;
            }
        }

        IrDebugAssert(len < block_size_, "");

        RefillBuffer();

        if (usize read = ReadBuffered(ptr, len))
        {
            ptr = static_cast<u8*>(ptr) + read;
            len -= read;
            total += read;
        }

        return total;
    }

    inline void EcbCipherStream::RefillBuffer()
    {
        buffered_ = input_->Read(buffer_, block_size_);

        if (buffered_ == block_size_)
        {
            cipher_->Update(buffer_, block_size_);
        }
        else if (buffered_ != 0)
        {
            std::memmove(&buffer_[block_size_ - buffered_], &buffer_[0], buffered_);
        }
    }

    inline usize EcbCipherStream::ReadBuffered(void* ptr, usize len)
    {
        if (buffered_ == 0)
            return 0;

        usize const read = std::min(len, buffered_);

        std::memcpy(ptr, &buffer_[block_size_ - buffered_], read);

        buffered_ -= read;

        ptr = static_cast<u8*>(ptr) + read;
        len -= read;

        return read;
    }
} // namespace Iridium
