#include "bulk.h"

namespace Iridium
{
    BulkStream::BulkStream(Rc<Stream> handle)
        : input_(handle)
    { }

    StreamPosition BulkStream::Seek(i64 offset, SeekWhence whence)
    {
        switch (whence)
        {
            case SeekWhence::Set: break;
            case SeekWhence::Cur: offset += here_; break;
            case SeekWhence::End: offset += input_->Size().get(0); break;
        }

        if (here_ < 0)
            here_ = -1;

        return StreamPosition::Checked(here_);
    }

    StreamPosition BulkStream::Tell()
    {
        return StreamPosition::Checked(here_);
    }

    StreamPosition BulkStream::Size()
    {
        return input_->Size();
    }

    usize BulkStream::Read(void* ptr, usize len)
    {
        if (here_ < 0)
            return 0;

        usize result = ReadInternal(ptr, len, here_);

        here_ += result;

        return result;
    }

    usize BulkStream::ReadBulk(void* ptr, usize len, u64 offset)
    {
        return ReadInternal(ptr, len, offset);
    }

    bool BulkStream::IsBulkSync()
    {
        return input_->IsBulkSync();
    }

    Rc<Stream> BulkStream::GetBulkStream(u64&, u64)
    {
        return input_;
    }

    usize BulkStream::ReadInternal(void* ptr, usize len, u64 offset)
    {
        return input_->ReadBulk(ptr, len, offset);
    }
} // namespace Iridium
