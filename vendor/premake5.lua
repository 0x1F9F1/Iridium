require "qt"

VENDOR_DIR = os.getcwd()

FMT_DIR = path.join(VENDOR_DIR, "fmt")
SPDLOG_DIR = path.join(VENDOR_DIR, "spdlog")
ZLIB_DIR = path.join(VENDOR_DIR, "zlib")
HEDLEY_DIR = path.join(VENDOR_DIR, "hedley")

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

project "*"
    qtprefix "Qt5"

    filter "architecture:x86"
        qtpath "C:/Qt/5.12.6/msvc2017"

    filter "architecture:x86_64"
        qtpath "C:/Qt/5.12.6/msvc2017_64"

    filter "configurations:Debug"
        qtsuffix "d"

    filter {}

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
