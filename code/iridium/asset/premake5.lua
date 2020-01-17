iridium_component "IrAsset"
    files {
        "**.cpp",
        "**.h",
    }

    filter "system:not Windows"
        excludes { "platform/win32_io.cpp" }
    filter {}

    includeZlib()

    links { "zlib" }
