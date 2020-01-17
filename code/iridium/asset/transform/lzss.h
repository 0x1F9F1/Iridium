#pragma once

#include "asset/transform.h"

namespace Iridium
{
    class LzssTransform : public BinaryTransform
    {
    public:
        LzssTransform();

        bool Reset() override;
        bool Update() override;

    private:
        u16 format_ {0};

        u8 buffered_ {0};
        u8 buffer_[2];

        u16fast current_ {0};
        u16fast pending_ {0};
        u8 window_[0x1000];

        void FlushPending();
    };
} // namespace Iridium
