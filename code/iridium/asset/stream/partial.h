#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class PartialStream final : public Stream
    {
    public:
        PartialStream(u64 start, u64 size, const Rc<Stream>& handle);

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, u64 offset) override;

        bool IsBulkSync() const override;

    protected:
        Rc<Stream> GetBulkStream(u64& offset, u64 size) override;

    private:
        u64 start_ {0};
        u64 size_ {0};

        u64 here_ {0};

        Rc<Stream> input_ {nullptr};

        usize ReadInternal(void* ptr, usize len, u64 offset);
    };
} // namespace Iridium
