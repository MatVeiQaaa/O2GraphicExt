#include "O2GraphicExt.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <psapi.h>
#include <math.h>

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

uintptr_t hOtwo = NULL;

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
	int frameCount;
};

struct Resource
{
	uintptr_t* vTable;
	BYTE unk1[0xC];
	DWORD idc;
	BYTE unk2[0x44];
	Data* data;
};

typedef uintptr_t(__thiscall* sub_4690)(DWORD* pThis, DWORD arg1, char* resName, DWORD arg3, DWORD arg4);
sub_4690 Sub_4690 = NULL;

uintptr_t __fastcall OnSub_4690(DWORD* pThis, DWORD edx, DWORD arg1, char* resName, DWORD arg3, DWORD arg4)
{
	if (strncmp(resName, "Test", 4) == 0)
	{
		arg4 = 0; // band-aid to fix 50% crash chance on resource init. I have no idea what arg4 means, or what this function does exactly.
	}
	return Sub_4690(pThis, arg1, resName, arg3, arg4);
}

typedef void(__thiscall* scaleOjt)(OJT* ojt, float scale);
scaleOjt ScaleOjt = NULL;

typedef uintptr_t(__thiscall* loadSceneElement)(DWORD* buffer, DWORD idc, ResDetails* resDetails);
loadSceneElement LoadSceneElement = NULL;

int comboCentrePoint = 72;
int comboDigitWidth = 44;
int comboDigitsSpacing = 0;
bool comboSmallNumberCorrections = true;
Resource* testRes;
Resource* test2Res;
uintptr_t __fastcall OnLoadSceneElement(DWORD* buffer, DWORD edx, DWORD idc, ResDetails* resDetails)
{
	if (strncmp(resDetails->name, "Test.ojs", 8) == 0)
	{
		ResDetails* testDetails = new ResDetails;
		testDetails->idc = 0x1B500000;
		strcpy(testDetails->name, "Test.ojs");
		uintptr_t result = LoadSceneElement(buffer, 0x1B500000, testDetails);
		Resource* resource = (Resource*)buffer;
		return result;
	}
	if (strncmp(resDetails->name, "Test2.ojs", 9) == 0)
	{
		ResDetails* testDetails = new ResDetails;
		testDetails->idc = 0x1B500000;
		strcpy(testDetails->name, "Test2.ojs");
		uintptr_t result = LoadSceneElement(buffer, 0x1B500000, testDetails);
		Resource* resource = (Resource*)buffer;
		return result;
	}
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

		if (strncmp(resDetails->name, "Note_ComboNum", 13) == 0)
		{
			element[resDetails->name]["Extra"] =
			{
				{"CentrePoint", 72},
				{"DigitWidth", 44},
				{"DigitsSpacing", 0},
				{"SmallNumberCorrections", true}
			};
		}

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
		if (strncmp(resDetails->name, "Note_ComboNum", 13) == 0)
		{
			cfgInFile.open("O2GraphicExt.json");
			config = json::parse(cfgInFile);
			comboCentrePoint = config[resDetails->name]["Extra"]["CentrePoint"];
			comboDigitWidth = config[resDetails->name]["Extra"]["DigitWidth"];
			comboDigitsSpacing = config[resDetails->name]["Extra"]["DigitsSpacing"];
			comboSmallNumberCorrections = config[resDetails->name]["Extra"]["SmallNumberCorrections"];
			cfgInFile.close();
		}
	}

	return result;
}

typedef int(__thiscall* drawCombo)(DWORD* pThis, int combo, int x, int y);
drawCombo DrawCombo = NULL;

int __fastcall OnDrawCombo(DWORD* pThis, DWORD edx, int combo, int x, int y)
{
	int selectedMap = *(int*)(hOtwo + 0x24D88);
	char* offsetTens = (char*)(hOtwo + 0x287F6);
	char* offsetHundreds = (char*)(hOtwo + 0x287EB);
	int* offsetThousands = (int*)(hOtwo + 0x287E5);
	char* oneCorrection = (char*)(hOtwo + 0x2886A);
	char* twoCorrectionOpcode = (char*)(hOtwo + 0x2887F);
	int gap = -comboDigitWidth - comboDigitsSpacing;
	int digitsCount = combo > 9 ? (int)log10((double)combo) + 1 : 1;
	x = comboCentrePoint + (comboDigitWidth + comboDigitsSpacing) / 2 * digitsCount;
	*offsetTens = gap;
	*offsetHundreds = gap * 2;
	*offsetThousands = gap * 3;
	/*0x2886A - increase digit X position for number 1 as 1 - byte unsigned int. (Default = 5)
	0x2887F - increase digit X position by 1 for number 2 as opcode 0x42 (inc edx), can replace for 0x90 to disable.*/
	if (comboSmallNumberCorrections)
	{
		*oneCorrection = 5;
		*twoCorrectionOpcode = 0x42;
	}
	else
	{
		*oneCorrection = 0;
		*twoCorrectionOpcode = 0x90;
	}

	return DrawCombo(pThis, combo, x, y);
}

typedef uintptr_t(__thiscall* loadRes)(DWORD* pThis, ResDetails* resDetails, DWORD* set);
loadRes LoadRes = NULL;

//Resource* testRes;
bool firstTimeForPlaying = true;
uintptr_t __fastcall OnLoadRes(DWORD* pThis, DWORD edx, ResDetails* resDetails, DWORD* set)
{
	uintptr_t returnStuff = LoadRes(pThis, resDetails, set);
	if (firstTimeForPlaying && *previousState == 6 && *currentState == 11)
	{
		ResDetails testDetails;
		testDetails.idc = 0x1B500000;
		strcpy(testDetails.name, "Test.ojs");
		testRes = (Resource*)LoadRes(pThis, &testDetails, 0);
		testRes->data->currentFrame = 1;
		testRes->data->frameCount = 1;
		strcpy(testDetails.name, "Test2.ojs");
		test2Res = (Resource*)LoadRes(pThis, &testDetails, 0);
		test2Res->data->currentFrame = 1;
		test2Res->data->frameCount = 1;
		std::stringstream sstream;
		sstream << "testRes: " << std::hex << testRes << '\n'
			<< "test2Res: " << test2Res;
		Logger(sstream.str());
		firstTimeForPlaying = false;
	}
	if (*previousState == 11)
	{
		firstTimeForPlaying = true;
	}
	return returnStuff;
}

typedef int(__thiscall* initPlayingScene)(DWORD* pThis, DWORD unk);
initPlayingScene InitPlayingScene = NULL;
int __fastcall OnInitPlayingScene(DWORD* pThis, DWORD edx, DWORD unk)
{

	InitPlayingScene(pThis, unk);
	ResDetails testDetails;
	testDetails.idc = 0x1B500000;
	strcpy(testDetails.name, "Test.ojs");
	testRes = (Resource*)LoadRes(pThis, &testDetails, 0);
	testRes->data->currentFrame = 1;
	testRes->data->frameCount = 1;
	strcpy(testDetails.name, "Test2.ojs");
	test2Res = (Resource*)LoadRes(pThis, &testDetails, 0);
	test2Res->data->currentFrame = 1;
	test2Res->data->frameCount = 1;
	std::stringstream sstream;
	sstream << "testRes: " << std::hex << testRes << '\n'
		<< "test2Res: " << test2Res;
	Logger(sstream.str());
	return 1;
}

typedef int(__thiscall* renderData)(Data* data, DWORD unk);
renderData RenderData = NULL;

typedef int(__thiscall* renderBg)(DWORD* pThis);
renderBg RenderBg = NULL;

int __fastcall OnRenderBg(DWORD* pThis, DWORD edx)
{
	
	int result = RenderBg(pThis);

	if (testRes != NULL)
	{
		RenderData(testRes->data, 0);
	}
	if (test2Res != NULL)
	{
		RenderData(test2Res->data, 0);
	}

	return result;
}

int O2GraphicExt::init(HMODULE hModule)
{
	Sleep(5000);
	hOtwo = (uintptr_t)GetModuleHandle("OTwo.exe");
	MODULEINFO modOtwo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hOtwo, &modOtwo, sizeof(MODULEINFO));
	using namespace sigmatch_literals;
	sigmatch::this_process_target target;
	sigmatch::search_context context = target.in_module("OTwo.exe");

	LoadSceneElement = (loadSceneElement)((uintptr_t)hOtwo + 0x05B0C0);
	LoadRes = (loadRes)((uintptr_t)hOtwo + 0x0303A0);
	Sub_4690 = (sub_4690)((uintptr_t)hOtwo + 0x4690);
	InitPlayingScene = (initPlayingScene)((uintptr_t)hOtwo + 0x024B60);
	RenderBg = (renderBg)((uintptr_t)hOtwo + 0x027D50);
	RenderData = (renderData)((uintptr_t)hOtwo + 0x010B90);
	ScaleOjt = (scaleOjt)((uintptr_t)hOtwo + 0x108C0);
	previousState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x4C });
	currentState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x50 });

	DrawCombo = (drawCombo)((uintptr_t)hOtwo + 0x028780);
	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect((void*)(hOtwo + 0x2886A), 160, PAGE_EXECUTE_READWRITE, &curProtection);

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

	if (MH_CreateHookEx((LPVOID)Sub_4690, &OnSub_4690, &Sub_4690) != MH_OK)
	{
		Logger("Couldn't hook Sub_4690");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)DrawCombo, &OnDrawCombo, &DrawCombo) != MH_OK)
	{
		Logger("Couldn't hook DrawCombo");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)LoadRes, &OnLoadRes, &LoadRes) != MH_OK)
	{
		Logger("Couldn't hook InitPlayingScene");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)RenderBg, &OnRenderBg, &RenderBg) != MH_OK)
	{
		Logger("Couldn't hook RenderBg");
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

