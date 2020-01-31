#include "oodle.h"

namespace Iridium
{
    OodleTransform::OodleTransform(i64 size)
        : output_size_(static_cast<usize>(size))
    {
        IrAssert(size >= 0 && size <= SIZE_MAX, "Ooodle cannot handle more than SIZE_MAX bytes");

        usize length = OodleLZDecoder_MemorySizeNeeded(OodleLZ_Compressor_Invalid, output_size_);

        state_ = operator new(length);

        decoder_ = OodleLZDecoder_Create(OodleLZ_Compressor_Invalid, output_size_, state_, length);
    }

    OodleTransform::~OodleTransform()
    {
        OodleLZDecoder_Destroy(decoder_);

        operator delete(state_);
    }

    bool OodleTransform::Reset()
    {
        current_out_ = nullptr;

        window_start_ = 0;
        window_end_ = 0;

        buffered_in_ = 0;
        need_in_ = 0;

        total_out_ = 0;

        return OodleLZDecoder_Reset(decoder_, 0, 0);
    }

    bool OodleTransform::Update()
    {
        while (true)
        {
            IrDebugAssert(window_start_ <= window_end_, "Invalid Window Position");

            if (usize buffered = window_end_ - window_start_)
            {
                if (current_out_ == buffer_out_)
                {
                    if (buffered > AvailOut)
                        buffered = AvailOut;

                    std::memcpy(NextOut, &buffer_out_[window_start_], buffered);

                    window_start_ += buffered;
                }
                else
                {
                    window_start_ = window_end_;
                }

                NextOut += buffered;
                AvailOut -= buffered;
                total_out_ += buffered;

                if (window_start_ == window_end_ && window_end_ == OODLELZ_BLOCK_LEN)
                {
                    window_start_ = 0;
                    window_end_ = 0;
                }

                if (AvailOut == 0)
                    return true;
            }

            if (AvailIn == 0)
                return true;

            if (window_end_ == 0)
            {
                IrDebugAssert(total_out_ <= output_size_, "Invalid Output Size");

                usize remaining = output_size_ - total_out_;

                if (remaining > OODLELZ_BLOCK_LEN)
                    remaining = OODLELZ_BLOCK_LEN;

                current_out_ = (AvailOut < remaining) ? buffer_out_ : NextOut;
            }

            if (buffered_in_)
            {
                IrDebugAssert(buffered_in_ <= need_in_, "Invalid Buffered In");

                usize needed = need_in_ - buffered_in_;

                if (needed >= AvailIn)
                    needed = AvailIn;

                std::memcpy(&buffer_in_[buffered_in_], NextIn, needed);

                buffered_in_ += needed;
                AvailIn -= needed;
                NextIn += needed;

                if (buffered_in_ < need_in_)
                    return true;

                OodleDecodeInfo info;

                if (OodleLZDecoder_DecodeSome(decoder_, &info, current_out_, window_end_, output_size_,
                        OODLELZ_BLOCK_LEN - window_end_, buffer_in_, buffered_in_, OodleLZ_FuzzSafe_No,
                        OodleLZ_CheckCRC_Yes, OodleLZ_Verbosity_None, OodleLZ_Decode_Unthreaded) <= 0)
                    return false;

                if (info.totalOut)
                {
                    window_end_ += info.totalOut;
                    buffered_in_ -= info.totalIn;
                    need_in_ = buffered_in_;

                    if (window_end_ >= AvailOut)
                        continue;
                }
                else
                {
                    buffered_in_ -= info.totalIn;

                    if (buffered_in_ > 0 && info.totalIn > 0)
                        std::memcpy(buffer_in_, &buffer_in_[info.totalIn], buffered_in_);

                    need_in_ = info.nextIn;

                    continue;
                }
            }

            while (window_end_ < OODLELZ_BLOCK_LEN && AvailIn)
            {
                OodleDecodeInfo info;

                if (OodleLZDecoder_DecodeSome(decoder_, &info, current_out_, window_end_, output_size_,
                        OODLELZ_BLOCK_LEN - window_end_, NextIn, AvailIn, OodleLZ_FuzzSafe_No, OodleLZ_CheckCRC_Yes,
                        OodleLZ_Verbosity_None, OodleLZ_Decode_Unthreaded) <= 0)
                    return false;

                NextIn += info.totalIn;
                AvailIn -= info.totalIn;

                if (info.totalOut)
                {
                    window_end_ += info.totalOut;

                    if (window_end_ >= AvailOut)
                        break;
                }
                else
                {
                    need_in_ = info.nextIn;

                    if (need_in_ == 0)
                        need_in_ = AvailIn + 1;

                    std::memcpy(buffer_in_, NextIn, AvailIn);
                    buffered_in_ = AvailIn;

                    NextIn += AvailIn;
                    AvailIn = 0;
                }
            }
        }
    }
} // namespace Iridium
