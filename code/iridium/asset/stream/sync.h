#pragma once

#include "asset/stream.h"
#include "core/mutex.h"

namespace Iridium
{
    class SyncStream final : public Stream
    {
    public:
        SyncStream(Rc<Stream> input);

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, u64 offset) override;

        bool IsBulkSync() const override;
        bool IsFullSync() const override;

    private:
        Rc<Stream> input_ {nullptr};
        Mutex lock_;
    };
} // namespace Iridium
