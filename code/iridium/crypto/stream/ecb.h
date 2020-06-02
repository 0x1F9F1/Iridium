#pragma once

#include "asset/stream.h"

namespace Iridium
{
    class Cipher;

    class EcbCipherStream final : public Stream
    {
    public:
        EcbCipherStream(Rc<Stream> input, Ptr<Cipher> cipher);
        ~EcbCipherStream() override;

        StreamPosition Seek(i64 offset, SeekWhence whence) override;

        StreamPosition Tell() override;
        StreamPosition Size() override;

        usize Read(void* ptr, usize len) override;

    private:
        Rc<Stream> input_ {nullptr};

        Ptr<Cipher> cipher_ {nullptr};
        usize block_size_ {0};

        usize padding_ {0};
        usize buffered_ {0};
        u8 buffer_[32];

        void RefillBuffer();
        usize ReadBuffered(void* ptr, usize len);
    };
} // namespace Iridium
