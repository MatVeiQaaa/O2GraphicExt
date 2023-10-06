// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <assert.h>
#include "helpers.hpp"
#include "O2GraphicExt.hpp"

static FILE* console_thread = nullptr;

DWORD WINAPI mainThread(HMODULE hModule)
{
    O2GraphicExt::init(hModule);

    while (true)
    {
        if (GetAsyncKeyState(VK_INSERT))
        {
            break;
        }
        Sleep(10);
    }

    O2GraphicExt::exit();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    HANDLE hThread;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifdef _DEBUG

        AllocConsole();
        freopen_s(&console_thread, "CONOUT$", "w", stdout);
#endif
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, hModule, 0, NULL);
        assert(hThread != NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if (lpReserved != nullptr) {
                break;
        }

#ifdef _DEBUG
        fclose(console_thread);
        FreeConsole();
#endif

        break;
    }
    return TRUE;
}

