iridium_component "IrCore"
    files {
        "**.cpp",
        "**.h",
    }

    filter "system:not Windows"
        excludes { "platform/minwin.h" }
    filter {}

    includeHedley()
    includeFmt()
    includeSpdlog()

    links { "spdlog", "fmt" }
