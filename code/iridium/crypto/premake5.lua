iridium_component "IrCrypto"
    files {
        "**.cpp",
        "**.h",
    }

    includeWolfSSL()

    links { "wolfssl" }
