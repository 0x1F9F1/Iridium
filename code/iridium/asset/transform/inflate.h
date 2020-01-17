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
} // namespace Iridium
