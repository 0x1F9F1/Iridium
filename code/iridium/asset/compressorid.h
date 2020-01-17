#pragma once

namespace Iridium
{
    enum class CompressorId : u8
    {
        Stored,
        Deflate,
        LZSS,
        LZXD,  // XCompress
        Oodle, // Leviathan, Kraken, Mermaid, Selkie

        // BZip2,
        // Deflated64,
        // LZMA,
    };
}
