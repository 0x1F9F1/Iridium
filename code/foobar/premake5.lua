include "../iridium/asset"

include "../angel"
-- include "../rage"
-- include "../real-virtuality"

project "FooBar"
    kind "ConsoleApp"

    -- filter { "system:Windows", "configurations:Final"}
    --     kind "WindowedApp"
    -- filter {}

    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Extra"

    files {
        "*.h",
        "*.cpp",
    }

    useIrPCH()
    includeIridium()

    links { "IrCore", "IrMain", "IrAsset", "IrAngel" }
    -- links { "IrAngel", "IrRage", "IrRealVirtuality" }
