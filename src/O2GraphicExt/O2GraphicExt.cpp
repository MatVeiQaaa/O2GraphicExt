#include "O2GraphicExt.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <psapi.h>

#include "helpers.hpp"

#include "minhook/include/MinHook.h"
#include "sigmatch/include/sigmatch/sigmatch.hpp"
#include "json/single_include/nlohmann/json.hpp"

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

int* previousState = NULL;
int* currentState = NULL;

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
	uintptr_t result = LoadSceneElement(buffer, idc, resDetails);
	Resource* resource = (Resource*)buffer;

	using json = nlohmann::ordered_json;
	std::ifstream cfgInFile;
	std::ofstream cfgOutFile;
	cfgInFile.open("O2GraphicExt.json");
	bool generateConfigData = 0;
	json config;
	if (cfgInFile.peek() == std::ifstream::traits_type::eof() || !cfgInFile.is_open())
	{
		cfgInFile.close();
		cfgOutFile.open("O2GraphicExt.json", std::ios_base::app);
		using json = nlohmann::json;
		json generateConfig; 
		generateConfig["generateConfig"] = true;
		cfgOutFile << std::setw(4) << generateConfig << std::endl;
		cfgOutFile.close();
	}

	cfgInFile.open("O2GraphicExt.json");
	config = json::parse(cfgInFile);
	cfgInFile.close();

	if (*previousState == 11 && config["generateConfig"] == true)
	{
		cfgOutFile.open("O2GraphicExt.json");
		config["generateConfig"] = false;
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}

	generateConfigData = config.at("generateConfig");

	if (generateConfigData && *currentState == 11 && resource->data->type == 3)
	{
		json element;
		/*element[resDetails->name]["Position"] = 
		{
				{"X", resource->data->X},
				{"Y", resource->data->Y}
		};*/
		
		element[resDetails->name]["Scale"] =
		{
			{"X", resource->data->ojt->XScale},
			{"Y", resource->data->ojt->YScale},
			{"Multiplier", resource->data->ojt->scaleAllRelative}
		};
		json finalJson;
		cfgInFile.open("O2GraphicExt.json");
		finalJson = json::parse(cfgInFile);
		finalJson.merge_patch(element);
		cfgInFile.close();
		cfgOutFile.open("O2GraphicExt.json");
		cfgOutFile << std::setw(4) << finalJson << std::endl;
		cfgOutFile.close();
	}
	
	if (!generateConfigData && *currentState == 11)
	{
		//resource->data->X = config[resDetails->name]["Position"]["X"];
		//resource->data->Y = config[resDetails->name]["Position"]["Y"];
		if (resource->data->type == 3)
		{
			resource->data->ojt->XScale = config[resDetails->name]["Scale"]["X"];
			resource->data->ojt->YScale = config[resDetails->name]["Scale"]["Y"];
			resource->data->ojt->scaleAllRelative = config[resDetails->name]["Scale"]["Multiplier"];
			ScaleOjt(resource->data->ojt, resource->data->ojt->scaleAllRelative);
		}
	}

	return result;
}

int O2GraphicExt::init(HMODULE hModule)
{
	Sleep(5000);
	uintptr_t hOtwo = (uintptr_t)GetModuleHandle("OTwo.exe");
	MODULEINFO modOtwo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hOtwo, &modOtwo, sizeof(MODULEINFO));
	using namespace sigmatch_literals;
	sigmatch::this_process_target target;
	sigmatch::search_context context = target.in_module("OTwo.exe");

	LoadSceneElement = (loadSceneElement)((uintptr_t)hOtwo + 0x05B0C0);
	ScaleOjt = (scaleOjt)((uintptr_t)hOtwo + 0x108C0);
	previousState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x4C });
	currentState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x50 });

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

