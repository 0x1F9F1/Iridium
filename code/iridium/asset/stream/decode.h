#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class BinaryTransform;

    class DecodeStream final : public Stream
    {
    public:
        DecodeStream(Rc<Stream> handle, Ptr<BinaryTransform> transform, u64 size, usize buffer_size = 0x2000);

        i64 Seek(i64 offset, SeekWhence whence) override;

        i64 Tell() override;
        i64 Size() override;

        usize Read(void* ptr, usize len) override;

    private:
        Rc<Stream> input_;
        Ptr<BinaryTransform> transform_;

        u64 size_ {0};
        i64 current_ {0};

        Ptr<u8[]> buffer_;
        usize buffer_size_ {0};
    };
} // namespace Iridium
