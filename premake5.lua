workspace "Iridium"
    location "build"

    flags "MultiProcessorCompile"

    configurations { "Debug", "Release", "Final" }
    platforms { "Win32", "Win64" }

    filter "kind:*App or SharedLib"
        targetdir "bin/%{prj.name}/%{cfg.platform}_%{cfg.buildcfg}"

    filter "configurations:Debug"
        optimize "Debug"
        symbols "On"
        defines { "_DEBUG", "IRIDIUM_DEBUG" }

    filter "configurations:Release"
        optimize "On"
        symbols "On"

    filter "configurations:Final"
        optimize "Full"
        symbols "FastLink"
        defines { "NDEBUG", "IRIDIUM_FINAL" }
        flags { "LinkTimeOptimization", "NoIncrementalLink" }

    filter "platforms:Win32"
        architecture "x32"

    filter "platforms:Win64"
        architecture "x86_64"

    filter "toolset:msc*"
        buildoptions { "/permissive-" }

    filter "toolset:msc-v*"
        buildoptions { "/Zc:throwingNew" }

    filter {}

group "Third Party"
    include "vendor"

group ""
    include "code"
