// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <assert.h>
#include <algorithm>
#include "helpers.hpp"
#include "O2GraphicExt.hpp"

static FILE* console_thread = nullptr;

DWORD WINAPI mainThread(HMODULE hModule)
{
    O2GraphicExt::init(hModule);

    while (true)
    {
    #ifdef _DEBUG
        if (GetAsyncKeyState(VK_INSERT))
        {
            break;
        }
    #endif
        Sleep(10);
    }

    O2GraphicExt::exit();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

extern "C" {
    __declspec(dllexport) bool addResource(char* name) {

        O2GraphicExt::addResourcesOutside.push_back(name);
        return true;
    }

    __declspec(dllexport) bool removeResource(char* name) {

        for (int i = 0; i < O2GraphicExt::addResourcesOutside.size(); i++)
        {
            if (O2GraphicExt::addResourcesOutside[i] == name)
            {
                O2GraphicExt::addResourcesOutside.erase(O2GraphicExt::addResourcesOutside.begin() + i);
                return true;
            }
        }
        return false;
    }

    __declspec(dllexport) Resource* getResource(char* name) {

        for (int i = 0; i < O2GraphicExt::addedResources.size(); i++)
        {
            if (strncmp(name, O2GraphicExt::addedResources[i]->data->frame->name, strlen(name)) == 0)
            {
                return O2GraphicExt::addedResources[i];
            }
        }
        return nullptr;
    }

    __declspec(dllexport) bool playAnimation(char* name, short type, int frameCount, int pauseDuration) {

        for (int i = 0; i < O2GraphicExt::addedResources.size(); i++)
        {
            if (strncmp(name, O2GraphicExt::addedResources[i]->data->frame->name, strlen(name)) == 0)
            {
                O2GraphicExt::addedResources[i]->data->currentFrame = 1;
                O2GraphicExt::addedResources[i]->data->unk3 = type;
                O2GraphicExt::addedResources[i]->data->frameCount = frameCount;
                O2GraphicExt::addedResources[i]->data->animationDuration = O2GraphicExt::addedResources[i]->data->animationCutoff = pauseDuration;
                return true;
            }
        }
        return false;
    }
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

