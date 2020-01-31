IRIDIUM_DIR = os.getcwd()

function includeIridium()
    includedirs { IRIDIUM_DIR }

    includeHedley()
    includeFmt()
    includeSpdlog()
end

function useIrPCH()
    pchsource(path.join(IRIDIUM_DIR, "irpch.cpp"))
    files { path.join(IRIDIUM_DIR, "irpch.cpp") }

    includedirs { IRIDIUM_DIR }
    pchheader("irpch.h")

    forceincludes { "irpch.h" }
end

function iridium_component(name)
    project(name)

    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    exceptionhandling "Off"
    rtti "Off"
    warnings "Extra"
    flags "FatalWarnings"

    useIrPCH()
    includeIridium()
end

project "*"
    defines { "SPDLOG_NO_EXCEPTIONS" }

include "core"
include "main"
include "asset"
include "crypto"
