#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class BulkStream final : public Stream
    {
    public:
        BulkStream(Rc<Stream> handle);

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, u64 offset) override;

        bool IsBulkSync() const override;

        Rc<Stream> GetBulkStream(u64& offset, u64 size) override;

    private:
        i64 here_ {0};

        Rc<Stream> input_ {nullptr};
    };
} // namespace Iridium
