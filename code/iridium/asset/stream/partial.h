#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class PartialStream final : public Stream
    {
    public:
        PartialStream(i64 start, i64 size, const Rc<Stream>& handle);

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;
        usize ReadBulk(void* ptr, usize len, i64 offset) override;

        bool IsBulkSync() override;

    protected:
        Rc<Stream> GetBulkStream(i64& offset, i64 size) override;

    private:
        i64 start_ {0};
        i64 size_ {0};

        i64 here_ {0};

        Rc<Stream> input_ {nullptr};

        usize ReadInternal(void* ptr, usize len, i64 offset);
    };
} // namespace Iridium
