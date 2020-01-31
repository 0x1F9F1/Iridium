#include "aes.h"

namespace Iridium
{
    AesEcbCipher::AesEcbCipher(const u8* key, usize key_length, bool decrypt)
        : decrypt_(decrypt)
    {
        IrAssert(wc_AesSetKey(&ctx_, key, static_cast<word32>(key_length), nullptr,
                     decrypt ? AES_DECRYPTION : AES_ENCRYPTION) == 0,
            "Failed to set key");
    }

    usize AesEcbCipher::Update(const u8* input, u8* output, usize length)
    {
        return ((decrypt_ ? wc_AesEcbDecrypt : wc_AesEcbEncrypt)(&ctx_, output, input, static_cast<word32>(length)) ==
                   0)
            ? length
            : 0;
    }

    usize AesEcbCipher::GetBlockSize()
    {
        return AES_BLOCK_SIZE;
    }
} // namespace Iridium
