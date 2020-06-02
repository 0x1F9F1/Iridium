VENDOR_DIR = os.getcwd()

FMT_DIR = path.join(VENDOR_DIR, "fmt")
SPDLOG_DIR = path.join(VENDOR_DIR, "spdlog")
ZLIB_DIR = path.join(VENDOR_DIR, "zlib")
HEDLEY_DIR = path.join(VENDOR_DIR, "hedley")
WOLFSSL_DIR = path.join(VENDOR_DIR, "wolfssl")
OODLE_DIR = path.join(VENDOR_DIR, "oodle")

function includeFmt()
    includedirs { path.join(FMT_DIR, "include") }
end

function includeSpdlog()
    includedirs { path.join(SPDLOG_DIR, "include") }
end

function includeZlib()
    includedirs { ZLIB_DIR }
end

function includeHedley()
    includedirs { HEDLEY_DIR }
end

function includeWolfSSL()
    includedirs { WOLFSSL_DIR }
end

function includeOodle()
    includedirs { path.join(OODLE_DIR, "include") }
end

project "fmt"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    files
    {
        path.join(FMT_DIR, "include/**.h"),
        path.join(FMT_DIR, "src/*.cc")
    }

    includedirs { path.join(FMT_DIR, "include") }

project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    files
    {
        path.join(SPDLOG_DIR, "include/**.h"),
        path.join(SPDLOG_DIR, "src/*.cpp")
    }

    includedirs { path.join(SPDLOG_DIR, "include") }

    includeFmt()

    links { "fmt" }

    defines { "SPDLOG_COMPILED_LIB", "SPDLOG_FMT_EXTERNAL" }

project "zlib"
    kind "StaticLib"
    language "C"
    warnings "Off"

    files
    {
        path.join(ZLIB_DIR, "*.h"),
        path.join(ZLIB_DIR, "*.c")
    }

project "*"
    defines { "ZLIB_CONST" }

project "wolfssl"
    kind "StaticLib"
    language "C"
    cdialect "C99"

    files
    {
        path.join(WOLFSSL_DIR, "wolfcrypt/src/aes.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/arc4.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/asn.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/blake2b.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/camellia.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/chacha.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/coding.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/compress.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/des3.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/dh.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/dsa.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/ecc.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/error.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/hc128.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/hmac.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/integer.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/logging.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/md2.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/md4.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/md5.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/memory.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/pkcs7.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/pkcs12.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/poly1305.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/pwdbased.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/rabbit.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/random.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/ripemd.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/rsa.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/sha.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/sha256.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/sha512.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/tfm.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/wc_port.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/wc_encrypt.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/hash.c"),
        path.join(WOLFSSL_DIR, "wolfcrypt/src/wolfmath.c"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/aes.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/arc4.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/asn.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/blake2.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/camellia.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/chacha.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/coding.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/compress.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/des3.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/dh.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/dsa.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/ecc.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/error-crypt.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/hc128.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/hmac.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/integer.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/logging.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/md2.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/md4.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/md5.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/memory.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/misc.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/pkcs7.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/poly1305.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/pwdbased.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/rabbit.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/random.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/ripemd.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/rsa.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/sha.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/sha256.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/sha512.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/tfm.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/wc_port.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/wc_encrypt.h"),
        path.join(WOLFSSL_DIR, "wolfssl/wolfcrypt/hash.h"),
    }

    includeWolfSSL()

project "*"
    defines { "WC_NO_HARDEN", "WOLFSSL_AES_DIRECT", "HAVE_AES_ECB" }

project "oodle"
    kind "StaticLib"
    language "C++"

    files
    {
        path.join(OODLE_DIR, "oodle.cpp"),
        path.join(OODLE_DIR, "include/*.h")
    }

    filter "architecture:x86"
        links { path.join(OODLE_DIR, "lib/x86/oo2core_7_win32.lib") }

    filter "architecture:x86_64"
        links { path.join(OODLE_DIR, "lib/x64/oo2core_8_win64.lib") }

    filter {}

    includeOodle()
