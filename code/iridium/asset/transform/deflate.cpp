#include "deflate.h"

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

    DeflateTransform::DeflateTransform(i32 window_bits, i32 level, i32 mem_level)
    {
        deflater_.zalloc = Z_alloc;
        deflater_.zfree = Z_free;

        deflateInit2(&deflater_, level, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY);
    }

    DeflateTransform::~DeflateTransform()
    {
        deflateEnd(&deflater_);
    }

    bool DeflateTransform::Reset()
    {
        Finished = false;

        return deflateReset(&deflater_) == Z_OK;
    }

    bool DeflateTransform::Update()
    {
        int error = Z_OK;

        while ((AvailIn || Finished) && AvailOut)
        {
            uInt actual_in = (AvailIn <= UINT_MAX) ? uInt(AvailIn) : UINT_MAX;
            uInt actual_out = (AvailOut <= UINT_MAX) ? uInt(AvailOut) : UINT_MAX;

            deflater_.next_in = NextIn;
            deflater_.next_out = NextOut;

            deflater_.avail_in = actual_in;
            deflater_.avail_out = actual_out;

            error = deflate(&deflater_, Finished ? Z_FINISH : Z_NO_FLUSH);

            AvailIn -= actual_in - deflater_.avail_in;
            AvailOut -= actual_out - deflater_.avail_out;

            NextIn = deflater_.next_in;
            NextOut = deflater_.next_out;

            if (error != Z_OK)
                break;
        }

        if (error == Z_STREAM_END)
        {
            Finished = true;

            return true;
        }

        if (error == Z_BUF_ERROR && AvailIn == 0)
        {
            return true;
        }

        return error == Z_OK;
    }
} // namespace Iridium
