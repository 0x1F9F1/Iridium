#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class BulkStream final : public Stream
    {
    public:
        BulkStream(Rc<Stream> handle);

        StreamPosition Seek(i64 offset, SeekWhence whence) override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, u64 offset) override;

        bool IsBulkSync() const override;

        Rc<Stream> GetBulkStream(u64& offset, u64 size) override;

    private:
        StreamPosition here_ {0};

        Rc<Stream> input_ {nullptr};
    };
} // namespace Iridium
