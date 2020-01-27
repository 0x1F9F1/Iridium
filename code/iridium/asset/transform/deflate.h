#pragma once

#include "asset/transform.h"

#include <zlib.h>

namespace Iridium
{
    class InflateTransform : public BinaryTransform
    {
    public:
        InflateTransform(i32 window_bits = -MAX_WBITS);
        ~InflateTransform() override;

        bool Reset() override;
        bool Update() override;

    private:
        z_stream inflater_ {};
    };

    class DeflateTransform : public BinaryTransform
    {
    public:
        DeflateTransform(i32 window_bits = -MAX_WBITS, i32 level = Z_BEST_COMPRESSION, i32 mem_level = MAX_MEM_LEVEL);
        ~DeflateTransform() override;

        bool Reset() override;
        bool Update() override;

    private:
        z_stream deflater_ {};
    };
} // namespace Iridium
