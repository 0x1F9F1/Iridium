#include "sync.h"

namespace Iridium
{
    SyncStream::SyncStream(Rc<Stream> input)
        : input_(std::move(input))
    {}

    i64 SyncStream::Seek(i64 offset, SeekWhence whence)
    {
        MutexGuard lock(lock_);

        return input_->Seek(offset, whence);
    }

    i64 SyncStream::Tell()
    {
        MutexGuard lock(lock_);

        return input_->Tell();
    }

    i64 SyncStream::Size()
    {
        MutexGuard lock(lock_);

        return input_->Size();
    }

    usize SyncStream::Read(void* ptr, usize len)
    {
        MutexGuard lock(lock_);

        return input_->Read(ptr, len);
    }

    usize SyncStream::ReadBulk(void* ptr, usize len, u64 offset)
    {
        MutexGuard lock(lock_);

        return input_->ReadBulk(ptr, len, offset);
    }

    bool SyncStream::IsBulkSync() const
    {
        return true;
    }

    bool SyncStream::IsFullSync() const
    {
        return true;
    }
} // namespace Iridium
