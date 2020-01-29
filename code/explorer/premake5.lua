qt = premake.extensions.qt

project "IridiumExplorer"
    qt.enable()

    kind "ConsoleApp"

    filter { "system:Windows", "configurations:Final"}
        kind "WindowedApp"

    filter {}

    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Extra"

    files {
        "*.h",
        "*.cpp",
        "*.ui",
        "*.rc",
        "*.qrc"
    }

    qtmodules {
        "core",
        "gui",
        "widgets"
    }

    useIrPCH()
    includeIridium()

    links { "IrCore", "IrMain", "IrAsset", "IrAngel", "IrRage" }
