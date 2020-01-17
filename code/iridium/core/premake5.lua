iridium_component "IrCore"
    files {
        "**.cpp",
        "**.h",
    }

    filter "system:not Windows"
        excludes { "platform/minwin.h" }
    filter {}

    includeFmt()
    includeSpdlog()

    links { "spdlog", "fmt" }
