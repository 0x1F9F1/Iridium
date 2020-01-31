#pragma once

#include "asset/transform.h"

#include <oodle.h>

namespace Iridium
{
    class OodleTransform : public BinaryTransform
    {
    public:
        OodleTransform(i64 size);
        ~OodleTransform();

        bool Reset() override;
        bool Update() override;

    private:
        void* state_ {nullptr};
        OodleLZDecoder* decoder_ {nullptr};

        usize output_size_ {0};
        u8* current_out_ {nullptr};

        usize window_start_ {0};
        usize window_end_ {0};

        usize buffered_in_ {0};
        usize need_in_ {0};

        usize total_out_ {0};

        u8 buffer_in_[OODLELZ_BLOCK_LEN];
        u8 buffer_out_[OODLELZ_BLOCK_LEN];
    };
} // namespace Iridium
