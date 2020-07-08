#include "bulk.h"

namespace Iridium
{
    BulkStream::BulkStream(Rc<Stream> handle)
        : input_(std::move(handle))
    {}

    i64 BulkStream::Seek(i64 offset, SeekWhence whence)
    {
        switch (whence)
        {
            case SeekWhence::Set: here_ = offset; break;
            case SeekWhence::Cur: here_ += offset; break;
            case SeekWhence::End: here_ = input_->Size() + offset; break;
        }

        return here_;
    }

    i64 BulkStream::Tell()
    {
        return here_;
    }

    i64 BulkStream::Size()
    {
        return input_->Size();
    }

    usize BulkStream::Read(void* ptr, usize len)
    {
        if (here_ < 0)
            return 0;

        usize result = input_->ReadBulk(ptr, len, here_);

        here_ += result;

        return result;
    }

    usize BulkStream::ReadBulk(void* ptr, usize len, u64 offset)
    {
        return input_->ReadBulk(ptr, len, offset);
    }

    bool BulkStream::IsBulkSync() const
    {
        return input_->IsBulkSync();
    }

    Rc<Stream> BulkStream::GetBulkStream(u64&, u64)
    {
        return input_;
    }
} // namespace Iridium
