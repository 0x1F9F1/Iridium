#pragma once

#include <stdint.h>

extern "C"
{
    /*
    enum OodleError

    struct OodleLZ_CompressOptions

    OODLELZ_MAX_OFFSET
    OODLELZ_SEEKCHUNKLEN_MAX

    OODLE_ASYNC_HANDLE_PENDING
    OODLE_ASYNC_HANDLE_DONE
    OODLE_ASYNC_HANDLE_ERROR

    OodleLZ_Compress_WriteOOZ_Async_WorkFunc couldn't start OodleLZ_Compress_Async, OodleLZ_Compress_Wait_GetResult
    */

#define OODLELZ_LOCALDICTIONARYSIZE_MAX 0x40000000
#define OODLELZ_BLOCK_LEN 0x40000
#define OODLELZ_FAILED 0
#define OODLEAPI __stdcall

    enum OodleLZ_CompressionLevel
    {
        OodleLZ_CompressionLevel_Min,
        OodleLZ_CompressionLevel_None = 0,
        OodleLZ_CompressionLevel_SuperFast,
        OodleLZ_CompressionLevel_VeryFast,
        OodleLZ_CompressionLevel_Fast,
        OodleLZ_CompressionLevel_Normal,
        OodleLZ_CompressionLevel_Optimal1,
        OodleLZ_CompressionLevel_Optimal2,
        OodleLZ_CompressionLevel_Optimal3,
        OodleLZ_CompressionLevel_Optimal4,
        OodleLZ_CompressionLevel_Optimal5,
        OodleLZ_CompressionLevel_Max = OodleLZ_CompressionLevel_Optimal5,
    };

    enum OodleLZ_Compressor
    {
        OodleLZ_Compressor_Invalid = -1,

        OodleLZ_Compressor_LZH,
        OodleLZ_Compressor_LZHLW,
        OodleLZ_Compressor_LZNIB,
        OodleLZ_Compressor_None,
        OodleLZ_Compressor_LZB16,
        OodleLZ_Compressor_LZBLW,
        OodleLZ_Compressor_LZA,
        OodleLZ_Compressor_LZNA,
        OodleLZ_Compressor_Kraken,
        OodleLZ_Compressor_Mermaid,
        OodleLZ_Compressor_BitKnit,
        OodleLZ_Compressor_Selkie,
        OodleLZ_Compressor_Hydra,
        OodleLZ_Compressor_Leviathan,
    };

    enum OodleLZ_Verbosity
    {
        OodleLZ_Verbosity_None,
        OodleLZ_Verbosity_Max = 3, // Unknown Name
    };

    enum OodleLZ_FuzzSafe
    {
        OodleLZ_FuzzSafe_No,
        OodleLZ_FuzzSafe_Yes,
    };

    enum OodleLZ_CheckCRC
    {
        OodleLZ_CheckCRC_No,
        OodleLZ_CheckCRC_Yes,
    };

    enum OodleLZ_Decode_ThreadPhase
    {
        OodleLZ_Decode_ThreadPhase1 = 0x1,
        OodleLZ_Decode_ThreadPhase2 = 0x2,

        OodleLZ_Decode_Unthreaded = OodleLZ_Decode_ThreadPhase1 | OodleLZ_Decode_ThreadPhase2,
    };

    struct OodleLZ_CompressOptions
    {
        // OodleLZ_CompressOptions_Validate

        // dictionarySize
        // profile
        // maxHuffmansPerChunk (deprecated_maxHuffmansPerChunk)
        // spaceSpeedTradeoffBytes
        // seekChunkLen (>= OODLELZ_BLOCK_LEN)
        // seekChunkReset
        // offsetShift
        // matchTableSizeLog2
        // minMatchLen
        // makeLongRangeMatcher
        // sendQuantumCRCs

        int32_t dword0;
        int32_t dword4;
        int32_t dword8;
        int32_t dwordC;
        int32_t dword10;
        int32_t dword14;
        int32_t spaceSpeedTradeoffBytes;
        int32_t dword1C;
        int32_t dword20;
        int32_t dictionarySize; // (<= OODLELZ_LOCALDICTIONARYSIZE_MAX)
        int32_t dword28;
    };

    struct OodleDecodeInfo
    {
        uint32_t totalOut;
        uint32_t totalIn;
        uint32_t nextOut;
        uint32_t nextIn;
    };

    struct OodleLZDecoder;

    uint32_t OODLEAPI OodleLZDecoder_MemorySizeNeeded(OodleLZ_Compressor compressor, ptrdiff_t compSize);

    OodleLZDecoder* OODLEAPI OodleLZDecoder_Create(
        OodleLZ_Compressor compressor, int64_t compSize, void* scratch, ptrdiff_t scratchSize);

    bool OODLEAPI OodleLZDecoder_Reset(OodleLZDecoder* decoder, size_t offset, ptrdiff_t compSize);

    void OODLEAPI OodleLZDecoder_Destroy(OodleLZDecoder* decoder);

    int32_t OODLEAPI OodleLZDecoder_DecodeSome(OodleLZDecoder* decoder, OodleDecodeInfo* info, uint8_t* rawBuf,
        size_t rawBufStart, size_t wholeRawLen, size_t rawLen, const uint8_t* compBuf, size_t compLen,
        OodleLZ_FuzzSafe fuzzSafe, OodleLZ_CheckCRC checkCrc, OodleLZ_Verbosity verbosity,
        OodleLZ_Decode_ThreadPhase threadPhase);

    int32_t OODLEAPI OodleLZ_Decompress(const uint8_t* compBuf, size_t compAvail, uint8_t* rawBuf, size_t rawAvail,
        OodleLZ_FuzzSafe fuzzSafe, OodleLZ_CheckCRC checkCrc, OodleLZ_Verbosity verbosity, uint8_t* outBuf, size_t outLen, void* read_callback,
        void* read_context, void* scratch, ptrdiff_t scratchSize, OodleLZ_Decode_ThreadPhase threadPhase);
}
