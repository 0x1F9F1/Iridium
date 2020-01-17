#include "inflate.h"

#include "core/alloc.h"

namespace Iridium
{
    InflateTransform::InflateTransform(i32 window_bits)
    {
        inflater_.zalloc = Z_alloc;
        inflater_.zfree = Z_free;

        inflateInit2(&inflater_, window_bits);
    }

    InflateTransform::~InflateTransform()
    {
        inflateEnd(&inflater_);
    }

    bool InflateTransform::Reset()
    {
        Finished = false;

        return inflateReset(&inflater_) == Z_OK;
    }

    bool InflateTransform::Update()
    {
        int error = Z_OK;

        while (AvailIn && AvailOut)
        {
            uInt actual_in = (AvailIn <= UINT_MAX) ? uInt(AvailIn) : UINT_MAX;
            uInt actual_out = (AvailOut <= UINT_MAX) ? uInt(AvailOut) : UINT_MAX;

            inflater_.next_in = NextIn;
            inflater_.next_out = NextOut;

            inflater_.avail_in = actual_in;
            inflater_.avail_out = actual_out;

            error = inflate(&inflater_, Z_SYNC_FLUSH);

            AvailIn -= actual_in - inflater_.avail_in;
            AvailOut -= actual_out - inflater_.avail_out;

            NextIn = inflater_.next_in;
            NextOut = inflater_.next_out;

            if (error != Z_OK)
                break;
        }

        if (error == Z_STREAM_END)
        {
            Finished = true;

            return true;
        }

        return error == Z_OK;
    }
} // namespace Iridium
