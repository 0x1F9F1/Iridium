#pragma once

#include "crypto/buzhash.h"

namespace Iridium
{
    struct Tfit2Buz;
}

namespace Iridium::Rage
{
    extern const BuzEntry GTA5_PC_KEY_HASHES[101];
    extern const BuzEntry GTA5_PC_TABLE_HASHES[17][16];
    extern const BuzEntry GTA5_PS4_KEYS_HASH;

    extern const BuzEntry RDR2_PC_KEY_HASHES[164];
    extern const Tfit2Buz RDR2_PC_TFIT2_HASHES;

    extern const BuzEntry RDR2_TFIT_TABLE_HASHES[17][16];
    extern const BuzEntry RDR2_IV_HASH;

    extern const BuzEntry RDR2_PS4_KEY_HASHES[164];
} // namespace Iridium::Rage
