#include "O2GraphicExt.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <psapi.h>

#include "helpers.hpp"

#include "minhook/include/MinHook.h"
#include "sigmatch/include/sigmatch/sigmatch.hpp"

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

struct ResDetails
{
	DWORD idc;
	char name[256];
};

struct OJT
{
	uintptr_t* vTable;
	BYTE unk1[0x28];
	float XScale;
	float YScale;
	BYTE unk2[0x20];
	int currentFrame;
	BYTE unk3[0x4];
	float scaleAllRelative;
};

struct Frame
{
	uintptr_t* vTable;
	BYTE unk1[0x14];
	char name[256];
	short frameCount;
	short encoder;
	BYTE unk2[0x24];
	int YSize;
	int XSize;
};

struct Data
{
	uintptr_t* vTable;
	Frame* frame;
	BYTE unk1[0x4];
	OJT* ojt;
	BYTE unk2[0x4];
	short type; // 1 == ojs, 3 == ojt
	short unk3;
	int X;
	int Y;
	BYTE unk4[0x144];
	int currentFrame;
};

struct Resource
{
	uintptr_t* vTable;
	BYTE unk1[0xC];
	DWORD idc;
	BYTE unk2[0x44];
	Data* data;
};

typedef void(__thiscall* scaleOjt)(OJT* ojt, float scale);
scaleOjt ScaleOjt = NULL;

typedef uintptr_t(__thiscall* loadSceneElement)(DWORD* buffer, DWORD idc, ResDetails* resDetails);
loadSceneElement LoadSceneElement = NULL;

uintptr_t __fastcall OnLoadSceneElement(DWORD* buffer, DWORD edx, DWORD idc, ResDetails* resDetails)
{
	std::stringstream log;
	log << std::hex << idc << ": " << resDetails->name << " at address " << buffer;
	Logger(log.str());
	uintptr_t result = LoadSceneElement(buffer, idc, resDetails);
	Resource* resource = (Resource*)buffer;
	if (resource->data->type == 3 && strncmp(resDetails->name, "Note_ComboNum1.ojt", 256) == 0)
	{
		ScaleOjt(resource->data->ojt, 0.8f);
	}

	return result;
}

int O2GraphicExt::init(HMODULE hModule)
{
	uintptr_t hOtwo = (uintptr_t)GetModuleHandle("OTwo.exe");
	MODULEINFO modOtwo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hOtwo, &modOtwo, sizeof(MODULEINFO));
	Logger("\n New Launch");
	using namespace sigmatch_literals;
	sigmatch::this_process_target target;
	sigmatch::search_context context = target.in_module("OTwo.exe");

	LoadSceneElement = (loadSceneElement)((uintptr_t)hOtwo + 0x05B0C0);
	ScaleOjt = (scaleOjt)((uintptr_t)hOtwo + 0x108C0);

	if (MH_Initialize() != MH_OK)
	{
		Logger("Couldn't init MinHook");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)LoadSceneElement, &OnLoadSceneElement, &LoadSceneElement) != MH_OK)
	{
		Logger("Couldn't hook LoadSceneElement");
		return 1;
	}

	if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		Logger("Couldn't enable hooks");
		return 1;
	}

	return 0;
}

int O2GraphicExt::exit()
{
	if (MH_QueueDisableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		Logger("Couldn't disable hooks");
		return 1;
	}

	if (MH_Uninitialize() != MH_OK)
	{
		Logger("Couldn't uninit MinHook");
		return 1;
	}

	return 0;
}

