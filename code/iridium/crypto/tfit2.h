#pragma once

#include "buzhash.h"
#include "cipher.h"

namespace Iridium
{
    class Stream;
    class BufferedStream;

    struct Tfit2Buz
    {
        BuzEntry InitTables;

        struct Round
        {
            struct Block
            {
                BuzEntry Masks;
                BuzEntry Xor;
            };

            BuzEntry Lookup;
            Block Blocks[16];
        };

        Round Rounds[17];

        BuzEntry EndMasks;
        BuzEntry EndTables;
        BuzEntry EndXor;
    };

    struct Tfit2Context
    {
        u64 InitTables[16][256];

        struct Round
        {
            u64 Lookup[4096];

            struct Block
            {
                u64 Masks[16];
                u32 Xor;
            } Blocks[16];
        } Rounds[17];

        u64 EndMasks[16][8];
        u8 EndTables[16][256];
        u8 EndXor[16];

        bool Load(BufferedStream& input);
        bool Save(BufferedStream& output);

        void Find(BuzFinder& f, const Tfit2Buz& hashes);
    };

    class Tfit2CbcCipher final : public Cipher
    {
    public:
        Tfit2CbcCipher(const u64 keys[17][2], const u8 iv[16], const Tfit2Context* ctx);

        usize Update(const u8* input, u8* output, usize length) override;

        usize GetBlockSize() override;

    private:
        const u64 (*keys_)[2] {nullptr};
        const Tfit2Context* ctx_ {nullptr};
        u8 iv_[16] {};
    };
} // namespace Iridium
