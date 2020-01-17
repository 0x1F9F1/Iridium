iridium_component "IrMain"
    files {
        "**.cpp",
        "**.h",
    }

    filter "system:not Windows"
        excludes { "win32_main.cpp" }
    filter {}
