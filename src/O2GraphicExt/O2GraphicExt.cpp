#include "O2GraphicExt.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <psapi.h>
#include <math.h>
#include <chrono>

#include "helpers.hpp"

#include "minhook/include/MinHook.h"
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
short* MessageBoxFunction = NULL;
char* MessageBoxText = NULL;

std::vector<std::string> addResources;

typedef bool(__thiscall* sub_4690)(DWORD* pThis, DWORD arg1, char* resName, DWORD arg3, DWORD arg4);
sub_4690 Sub_4690 = NULL;

bool __fastcall OnSub_4690(DWORD* pThis, DWORD edx, DWORD arg1, char* resName, DWORD arg3, DWORD arg4)
{
	for (int i = 0; i < addResources.size(); i++)
	{
		if (strncmp(resName, addResources[i].c_str(), addResources[i].size()) == 0)
		{
			arg4 = 0; // band-aid to fix 50% crash chance on resource init. I have no idea what arg4 means, or what this function does exactly.
			if (!Sub_4690(pThis, arg1, resName, arg3, arg4))
			{
				*MessageBoxFunction = 0;
				std::string message = std::string(resName) + " missing from Playing.opi";
				strcpy(MessageBoxText, message.c_str());
				return false;
			}
			break;
		}
	}
	return Sub_4690(pThis, arg1, resName, arg3, arg4);
}

void GenerateElementConfig(Resource* resource, ResDetails* resDetails)
{
	if (*currentState == 11 && resource->data->type == 3)
	{
		using json = nlohmann::ordered_json;
		std::ifstream cfgInFile;
		std::ofstream cfgOutFile;
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
	uintptr_t result = LoadSceneElement(buffer, idc, resDetails);
	Resource* resource = (Resource*)buffer;

	using json = nlohmann::ordered_json;
	std::ifstream cfgInFile;
	cfgInFile.open("O2GraphicExt.json");
	json config;
	try {
		config = json::parse(cfgInFile);
	}
	catch (...) {
		config["addResources"] = std::vector<std::string>(NULL);
		std::ofstream cfgOutFile;
		cfgOutFile.open("O2GraphicExt.json");
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}
	cfgInFile.close();
	
	if (*currentState == 11)
	{
		//resource->data->X = config[resDetails->name]["Position"]["X"];
		//resource->data->Y = config[resDetails->name]["Position"]["Y"];
		try {
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
		catch (...) {
			GenerateElementConfig(resource, resDetails);
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
		std::ifstream cfgInFile;
		using json = nlohmann::ordered_json;
		json config;
		cfgInFile.open("O2GraphicExt.json");
		config = json::parse(cfgInFile);
		try {
			addResources = config["addResources"].get<std::vector<std::string>>();
		}
		catch (...) {
			config["addResources"] = std::vector<std::string>(NULL);
			std::ofstream cfgOutFile;
			cfgOutFile.open("O2GraphicExt.json");
			cfgOutFile << std::setw(4) << config << std::endl;
			cfgOutFile.close();
		}
		cfgInFile.close();
		addResources.insert(addResources.end(), O2GraphicExt::addResourcesOutside.begin(), O2GraphicExt::addResourcesOutside.end());

		for (int i = 0; i < addResources.size(); i++)
		{
			ResDetails testDetails;
			testDetails.idc = 0x1B500000;
			strcpy(testDetails.name, addResources[i].c_str());
			Resource* resource = (Resource*)LoadRes(pThis, &testDetails, 0);
			resource->data->currentFrame = 1;
			resource->data->frameCount = 1;
			resource->data->unk3 = 0;
			O2GraphicExt::addedResources.push_back(resource);
		}

		firstTimeForPlaying = false;
	}
	if (*previousState == 11)
	{
		addResources.clear();
		O2GraphicExt::addedResources.clear();
		firstTimeForPlaying = true;
	}
	return returnStuff;
}

typedef int(__thiscall* initPlayingScene)(DWORD* pThis, DWORD unk);
initPlayingScene InitPlayingScene = NULL;

typedef void(__thiscall* playDataAnimation)(Data* data, int playFor, int pauseFor, int arg3, int arg4);
playDataAnimation PlayDataAnimation = NULL;

typedef int(__thiscall* renderData)(Data* data, DWORD unk);
renderData RenderData = NULL;

typedef int(__thiscall* renderPlaying)(DWORD* pThis, DWORD arg1);
renderPlaying RenderPlaying = NULL;

std::chrono::steady_clock::time_point time_last;
double timeSinceLastAnim;

int __fastcall OnRenderPlaying(DWORD* pThis, DWORD edx, DWORD arg1)
{
	int result = RenderPlaying(pThis, arg1);

	if (!reinterpret_cast<long long&>(time_last))
	{
		time_last = std::chrono::steady_clock::now();
	}
	std::chrono::steady_clock::time_point time_now = std::chrono::steady_clock::now();
	double delta;
	timeSinceLastAnim += delta = std::chrono::duration_cast<std::chrono::microseconds>(time_now - time_last).count() / 1000000.0;
	time_last = time_now;

	if (O2GraphicExt::addedResources.empty()) return result;
	for (int i = 0; i < O2GraphicExt::addedResources.size(); i++)
	{
		if (timeSinceLastAnim >= 0.016) // 60fps normalization.
		{
			if (!O2GraphicExt::addedResources[i]->data->unk3) {
				RenderData(O2GraphicExt::addedResources[i]->data, 0);
				continue;
			}

			if (O2GraphicExt::addedResources[i]->data->currentFrame && O2GraphicExt::addedResources[i]->data->currentFrame < O2GraphicExt::addedResources[i]->data->frameCount)
			{
				O2GraphicExt::addedResources[i]->data->currentFrame++;
			}
			else if (O2GraphicExt::addedResources[i]->data->animationDuration == 0)
			{
				O2GraphicExt::addedResources[i]->data->animationDuration = O2GraphicExt::addedResources[i]->data->animationCutoff;
				O2GraphicExt::addedResources[i]->data->unk3 > 1 ? O2GraphicExt::addedResources[i]->data->currentFrame = 1 : O2GraphicExt::addedResources[i]->data->currentFrame = 0;
			}
			else O2GraphicExt::addedResources[i]->data->animationDuration--;

			if (i == O2GraphicExt::addedResources.size() - 1) timeSinceLastAnim = 0;
		}

		RenderData(O2GraphicExt::addedResources[i]->data, 0);
	}

	return result;
}

typedef int(__thiscall* renderBg)(DWORD* pThis);
renderBg RenderBg = NULL;

int __fastcall OnRenderBg(DWORD* pThis, DWORD edx)
{
	int result = RenderBg(pThis);

	if (!O2GraphicExt::addedResources.empty())
	{
		for (int i = 0; i < O2GraphicExt::addedResources.size(); i++)
		{
			RenderData(O2GraphicExt::addedResources[i]->data, 0);
		}
	}

	return result;
}

int O2GraphicExt::init(HMODULE hModule)
{
	hOtwo = (uintptr_t)GetModuleHandle("OTwo.exe");
	MODULEINFO modOtwo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hOtwo, &modOtwo, sizeof(MODULEINFO));

	LoadSceneElement = (loadSceneElement)((uintptr_t)hOtwo + 0x05B0C0);
	LoadRes = (loadRes)((uintptr_t)hOtwo + 0x0303A0);
	Sub_4690 = (sub_4690)((uintptr_t)hOtwo + 0x4690);
	InitPlayingScene = (initPlayingScene)((uintptr_t)hOtwo + 0x024B60);
	PlayDataAnimation = (playDataAnimation)((uintptr_t)hOtwo + 0x010DB0);
	RenderBg = (renderBg)((uintptr_t)hOtwo + 0x027D50);
	RenderPlaying = (renderPlaying)((uintptr_t)hOtwo + 0x027DD0);
	RenderData = (renderData)((uintptr_t)hOtwo + 0x010B90);
	ScaleOjt = (scaleOjt)((uintptr_t)hOtwo + 0x108C0);
	uintptr_t* stateManager = (uintptr_t*)(hOtwo + 0x1C8884);
	while (!*stateManager) Sleep(50); // wait for StateManager to initialize.
	previousState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x4C });
	currentState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x50 });
	MessageBoxFunction = (short*)FollowPointers(hOtwo, { 0x1C8884, 0x5C });
	MessageBoxText = (char*)FollowPointers(hOtwo, { 0x1C8884, 0x64 });

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

	if (MH_CreateHookEx((LPVOID)RenderPlaying, &OnRenderPlaying, &RenderPlaying) != MH_OK)
	{
		Logger("Couldn't hook PlayingLoop");
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

