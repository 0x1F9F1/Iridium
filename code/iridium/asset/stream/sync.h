#pragma once

#include "asset/stream.h"
#include "core/mutex.h"

namespace Iridium
{
    class SyncStream final : public Stream
    {
    public:
        SyncStream(Rc<Stream> input);

        StreamPosition Seek(i64 offset, SeekWhence whence) override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, u64 offset) override;

        bool IsBulkSync();
        bool IsFullSync();

    private:
        Rc<Stream> input_ {nullptr};
        Mutex lock_;
    };
} // namespace Iridium
