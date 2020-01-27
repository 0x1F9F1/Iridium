#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class BinaryTransform;

    class EncodeStream final : public Stream
    {
    public:
        EncodeStream(Rc<Stream> output, Ptr<BinaryTransform> transform, usize buffer_size = 0x2000);
        ~EncodeStream() override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Write(const void* ptr, usize len) override;

        bool Flush() override;

    private:
        Rc<Stream> output_;
        Ptr<BinaryTransform> transform_;

        i64 size_ {0};

        Ptr<u8[]> buffer_;
        usize buffer_size_ {0};
    };
} // namespace Iridium
