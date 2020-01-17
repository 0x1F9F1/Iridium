#include "core/main.h"
#include "core/platform/minwin.h"

#include <shellapi.h>

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    int argc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    char** argv = new char*[argc + 1];
    argv[argc] = nullptr;

    for (int i = 0; i < argc; ++i)
    {
        int wlen = lstrlenW(wargv[i]) + 1;
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], wlen, NULL, 0, NULL, NULL);
        argv[i] = new char[len];
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], wlen, argv[i], len, NULL, NULL);
    }

    LocalFree(wargv);
    wargv = nullptr;

    int result = Iridium::Main(argc, argv);

    for (int i = 0; i < argc; ++i)
        delete[] argv[i];

    delete[] argv;

    return result;
}
