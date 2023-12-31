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

std::string configPath = "O2GraphicExt.json";

int* previousState = NULL;
int* currentState = NULL;
short* MessageBoxType = NULL;
short* MessageBoxFunction = NULL;
char* MessageBoxText = NULL;
int noteplainX = 21;
int* noteplainRectWidth = nullptr;
int hitPosition = 480;

std::vector<std::string> addResources;

typedef uintptr_t(__cdecl* o2HeapAlloc)(unsigned int size);
o2HeapAlloc O2HeapAllocClean = nullptr;
o2HeapAlloc O2HeapAllocDirty = nullptr;

typedef int(__cdecl* o2HeapFree)(uintptr_t* buffer);
o2HeapFree O2HeapFree = nullptr;

typedef bool(__thiscall* sub_4690)(DWORD* pThis, DWORD arg1, char* resName, DWORD arg3, DWORD arg4);
sub_4690 Sub_4690 = NULL; // Called from LoadSceneRes.

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

int* counterCoolX = nullptr;
int* counterCoolY = nullptr;
int* counterGoodX = nullptr;
int* counterGoodY = nullptr;
int* counterBadX = nullptr;
int* counterBadY = nullptr;
int* counterMissX = nullptr;
int* counterMissY = nullptr;
int* counterComboX = nullptr;
int* counterComboY = nullptr;
int* counterScoreX = nullptr;
int* counterScoreY = nullptr;
int* indicatorVol1X = nullptr;
int* indicatorVol1Y = nullptr;
int* indicatorVol2X = nullptr;
int* indicatorVol2Y = nullptr;
int* indicatorNoteX = nullptr;
int* indicatorNoteY = nullptr;
int* indicatorVisX = nullptr;
int* indicatorVisY = nullptr;
int* indicatorArrX = nullptr;
int* indicatorArrY = nullptr;

int comboCentrePoint = 77;
int comboDigitWidth = 44;
int comboDigitsSpacing = 0;
bool comboSmallNumberCorrections = true;
int* comboStartingY = nullptr;
int* comboTargetY = nullptr;
int8_t* comboIncrementY = nullptr;

int8_t* comboTextIncrementX = nullptr;
int8_t* comboTextIncrementY = nullptr;

float* judgementInitialScale = nullptr;
float* judgementScaleCompounding = nullptr;

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
				{"SmallNumberCorrections", true},
				{"StartingYPosition", *comboStartingY},
				{"TargetYPosition", *comboTargetY},
				{"IncrementY", *comboIncrementY},
				{"TextIncrementX", *comboTextIncrementX},
				{"TextIncrementY", *comboTextIncrementY}
			};
		}
		if (strncmp(resDetails->name, "Note_Cool", 9) == 0)
		{
			element[resDetails->name]["Extra"] =
			{
				{"InitialScale", *judgementInitialScale},
				{"ScaleMultiplyPerFrame", *judgementScaleCompounding}
			};
		}

		json finalJson;
		cfgInFile.open(configPath);
		finalJson = json::parse(cfgInFile);
		finalJson.merge_patch(element);
		cfgInFile.close();
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << finalJson << std::endl;
		cfgOutFile.close();
	}
	else if (strncmp(resDetails->name, "Playing_BG", 10) == 0) // Using this for misc. options loading, as its the first element to load.
	{
		using json = nlohmann::ordered_json;
		std::ifstream cfgInFile;
		std::ofstream cfgOutFile;
		json element;

		element["Miscellaneous"]["Counters"]["Cool"] =
		{
			{"X", *counterCoolX},
			{"Y", *counterCoolY}
		};
		element["Miscellaneous"]["Counters"]["Good"] =
		{
			{"X", *counterGoodX},
			{"Y", *counterGoodY}
		};
		element["Miscellaneous"]["Counters"]["Bad"] =
		{
			{"X", *counterBadX},
			{"Y", *counterBadY}
		};
		element["Miscellaneous"]["Counters"]["Miss"] =
		{
			{"X", *counterMissX},
			{"Y", *counterMissY}
		};
		element["Miscellaneous"]["Counters"]["Combo"] =
		{
			{"X", *counterComboX},
			{"Y", *counterComboY}
		};
		element["Miscellaneous"]["Counters"]["Score"] =
		{
			{"X", *counterScoreX},
			{"Y", *counterScoreY}
		};
		element["Miscellaneous"]["Indicators"]["Vol1"] =
		{
			{"X", *indicatorVol1X},
			{"Y", *indicatorVol1Y}
		};
		element["Miscellaneous"]["Indicators"]["Vol2"] =
		{
			{"X", *indicatorVol2X},
			{"Y", *indicatorVol2Y}
		};
		element["Miscellaneous"]["Indicators"]["NoteType"] =
		{
			{"X", *indicatorNoteX},
			{"Y", *indicatorNoteY}
		};
		element["Miscellaneous"]["Indicators"]["RingVis"] =
		{
			{"X", *indicatorVisX},
			{"Y", *indicatorVisY}
		};
		element["Miscellaneous"]["Indicators"]["RingArr"] =
		{
			{"X", *indicatorArrX},
			{"Y", *indicatorArrY}
		};

		json finalJson;
		cfgInFile.open(configPath);
		finalJson = json::parse(cfgInFile);
		finalJson.merge_patch(element);
		cfgInFile.close();
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << finalJson << std::endl;
		cfgOutFile.close();
	}
}

typedef void(__thiscall* scaleOjt)(OJT* ojt, float scale);
scaleOjt ScaleOjt = NULL;

typedef void(__thiscall* setPosition)(Data* data, int posX, int posY);
setPosition SetPosition = NULL;

void __fastcall OnSetPosition(Data* data, DWORD edx, int posX, int posY)
{
	if (*currentState == 11)
	{
		if (data->type == 3 && (strncmp(data->ojt->frame->name, "Note_ComboNum", 13) == 0)) return SetPosition(data, posX, posY);
		if (data->type == 3 && (strncmp(data->ojt->frame->name, "Note_Combo", 10) == 0 ||
			strncmp(data->ojt->frame->name, "Note_Cool", 9) == 0 ||
			strncmp(data->ojt->frame->name, "Note_Good", 9) == 0 ||
			strncmp(data->ojt->frame->name, "Note_Bad", 8) == 0 ||
			strncmp(data->ojt->frame->name, "Note_Miss", 9) == 0
			)
			)
		{
			posX = posX + noteplainX - 21 + (*noteplainRectWidth - 193) / 2;
		}
		if (data->type == 1 && (strncmp(data->frame->name, "Note_Jam.ojs", 12) == 0 ||
			strncmp(data->frame->name, "Life_gauge_back.ojs", 19) == 0 ||
			strncmp(data->frame->name, "Life_gauge_bar.ojs", 18) == 0 || 
			strncmp(data->frame->name, "Note_BG.ojs", 11) == 0
			)
			)
		{
			if (strncmp(data->frame->name, "Note_Jam.ojs", 12) == 0) posX = posX + noteplainX - 21 + (*noteplainRectWidth - 193) / 2;
			else posX = posX + noteplainX - 21 + (*noteplainRectWidth - 193);
		}
	}
	return SetPosition(data, posX, posY);
}

typedef uintptr_t(__thiscall* loadSceneElement)(DWORD* buffer, DWORD idc, ResDetails* resDetails);
loadSceneElement LoadSceneElement = NULL; // Called from LoadRes.

uintptr_t __fastcall OnLoadSceneElement(DWORD* buffer, DWORD edx, DWORD idc, ResDetails* resDetails)
{
	uintptr_t result = LoadSceneElement(buffer, idc, resDetails);
	Resource* resource = (Resource*)buffer;

	using json = nlohmann::ordered_json;
	std::ifstream cfgInFile;
	cfgInFile.open(configPath);
	json config;
	try {
		config = json::parse(cfgInFile);
	}
	catch (...) {
		config["addResources"] = std::vector<std::string>(NULL);
		std::ofstream cfgOutFile;
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}
	cfgInFile.close();
	
	if (*currentState == 11)
	{
		try {
			if (strncmp(resDetails->name, "Playing_BG", 10) == 0)
			{
				*counterCoolX = config["Miscellaneous"]["Counters"]["Cool"]["X"];
				*counterCoolY = config["Miscellaneous"]["Counters"]["Cool"]["Y"];
				*counterGoodX = config["Miscellaneous"]["Counters"]["Good"]["X"];
				*counterGoodY = config["Miscellaneous"]["Counters"]["Good"]["Y"];
				*counterBadX = config["Miscellaneous"]["Counters"]["Bad"]["X"];
				*counterBadY = config["Miscellaneous"]["Counters"]["Bad"]["Y"];
				*counterMissX = config["Miscellaneous"]["Counters"]["Miss"]["X"];
				*counterMissY = config["Miscellaneous"]["Counters"]["Miss"]["Y"];
				*counterComboX = config["Miscellaneous"]["Counters"]["Combo"]["X"];
				*counterComboY = config["Miscellaneous"]["Counters"]["Combo"]["Y"];
				*counterScoreX = config["Miscellaneous"]["Counters"]["Score"]["X"];
				*counterScoreY = config["Miscellaneous"]["Counters"]["Score"]["Y"];
				*indicatorVol1X = config["Miscellaneous"]["Indicators"]["Vol1"]["X"];
				*indicatorVol1Y = config["Miscellaneous"]["Indicators"]["Vol1"]["Y"];
				*indicatorVol2X = config["Miscellaneous"]["Indicators"]["Vol2"]["X"];
				*indicatorVol2Y = config["Miscellaneous"]["Indicators"]["Vol2"]["Y"];
				*indicatorNoteX = config["Miscellaneous"]["Indicators"]["NoteType"]["X"];
				*indicatorNoteY = config["Miscellaneous"]["Indicators"]["NoteType"]["Y"];
				*indicatorVisX = config["Miscellaneous"]["Indicators"]["RingVis"]["X"];
				*indicatorVisY = config["Miscellaneous"]["Indicators"]["RingVis"]["Y"];
				*indicatorArrX = config["Miscellaneous"]["Indicators"]["RingArr"]["X"];
				*indicatorArrY = config["Miscellaneous"]["Indicators"]["RingArr"]["Y"];
			}
			if (resource->data->type == 3)
			{
				/*resource->data->X = config[resDetails->name]["Position"]["X"];
				resource->data->Y = config[resDetails->name]["Position"]["Y"];*/
				resource->data->ojt->XScale = config[resDetails->name]["Scale"]["X"];
				resource->data->ojt->YScale = config[resDetails->name]["Scale"]["Y"];
				resource->data->ojt->scaleAllRelative = config[resDetails->name]["Scale"]["Multiplier"];
				ScaleOjt(resource->data->ojt, resource->data->ojt->scaleAllRelative);
			}
			if (strncmp(resDetails->name, "Note_ComboNum", 13) == 0)
			{
				comboCentrePoint = config[resDetails->name]["Extra"]["CentrePoint"];
				comboDigitWidth = config[resDetails->name]["Extra"]["DigitWidth"];
				comboDigitsSpacing = config[resDetails->name]["Extra"]["DigitsSpacing"];
				comboSmallNumberCorrections = config[resDetails->name]["Extra"]["SmallNumberCorrections"];
				*comboStartingY = config[resDetails->name]["Extra"]["StartingYPosition"];
				*comboTargetY = config[resDetails->name]["Extra"]["TargetYPosition"];
				*comboIncrementY = config[resDetails->name]["Extra"]["IncrementY"];
				*comboTextIncrementX = config[resDetails->name]["Extra"]["TextIncrementX"];
				*comboTextIncrementY = config[resDetails->name]["Extra"]["TextIncrementY"]; // Combo text animation stops when combo's TargetYPosition reached.
			}
			if (strncmp(resDetails->name, "Note_Cool", 9) == 0)
			{
				*judgementInitialScale = config[resDetails->name]["Extra"]["InitialScale"];
				*judgementScaleCompounding = config[resDetails->name]["Extra"]["ScaleMultiplyPerFrame"];
			}
		}
		catch (...) {
			GenerateElementConfig(resource, resDetails);
		}
	}

	return result;
}

typedef void(__thiscall* drawPill)(DWORD* pThis, int arg1, int x, int y);
drawPill DrawPill = NULL;

void __fastcall OnDrawPill(DWORD* pThis, DWORD edx, int arg1, int x, int y)
{
	return DrawPill(pThis, arg1, x + noteplainX - 21, y);
}

typedef int(__thiscall* drawJamCombo)(DWORD* pThis, int combo, int x, int y);
drawJamCombo DrawJamCombo = NULL; // Called from judgement animation function.

int __fastcall OnDrawJamCombo(DWORD* pThis, DWORD edx, int combo, int x, int y)
{
	return DrawJamCombo(pThis, combo, x + noteplainX - 21 + (*noteplainRectWidth - 193) / 2, y);
}

typedef int(__thiscall* drawCombo)(DWORD* pThis, int combo, int x, int y);
drawCombo DrawCombo = NULL; // Called from judgement animation function.

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
	x = noteplainX - 21 + (*noteplainRectWidth - 193) / 2 + comboCentrePoint + (comboDigitWidth + comboDigitsSpacing) / 2 * digitsCount;
	*offsetTens = gap;
	*offsetHundreds = gap * 2;
	*offsetThousands = gap * 3;
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
loadRes LoadRes = NULL; // Called from various scene loading functions.

bool firstTimeForPlaying = true;
uintptr_t __fastcall OnLoadRes(DWORD* pThis, DWORD edx, ResDetails* resDetails, DWORD* set)
{
	uintptr_t returnStuff = LoadRes(pThis, resDetails, set);
	if (firstTimeForPlaying && *previousState == 6 && *currentState == 11)
	{
		std::ifstream cfgInFile;
		using json = nlohmann::ordered_json;
		json config;
		cfgInFile.open(configPath);
		config = json::parse(cfgInFile);
		try {
			addResources = config["addResources"].get<std::vector<std::string>>();
		}
		catch (...) {
			config["addResources"] = std::vector<std::string>(NULL);
			std::ofstream cfgOutFile;
			cfgOutFile.open(configPath);
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

typedef int(__thiscall* initPlayingSceneAdditional)(DWORD* pThis, DWORD unk);
initPlayingSceneAdditional InitPlayingSceneAdditional = NULL;

typedef void(__thiscall* initPlayingScene)(DWORD* pThis, DWORD unk1, DWORD unk2);
initPlayingScene InitPlayingScene = NULL;

int* playingRectHeight = nullptr;
int* noteplainRectHeight = nullptr;
int* shortNoteOffset = nullptr;
int* lnBodyOffset = nullptr;
int* lnTailOffset = nullptr;
int* guideLineOffset = nullptr;
int* guidelinesOffset = nullptr;
int* measureLineOffset = nullptr;
int* lnHeadLimit = nullptr;
int* lnBodyLimit = nullptr;
int* noteRenderCutoff = nullptr;
int* unkHitpos1 = nullptr;
int* unkHitpos2 = nullptr;
int* unkHitpos3 = nullptr;
BYTE* noteThickness = nullptr;

bool showMeasurelines = true;
bool showTargetBar = true;
int* effectsY = nullptr;
std::vector<int*> effectOffset = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
std::vector<int*> noteOffset = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
std::vector<int> laneWidth;
std::vector<int> noteWidth;
int laneSpacing = 0;
bool lnTail = true;

void __fastcall OnInitPlayingScene(DWORD* pThis, DWORD edx, DWORD unk1, DWORD unk2)
{
	std::ifstream cfgInFile;
	using json = nlohmann::ordered_json;
	json config;
	cfgInFile.open("O2GraphicExt.json");
	config = json::parse(cfgInFile);
	cfgInFile.close();
	std::string currentSkin;
	try {
		currentSkin = config["SelectedSkin"].get<std::string>();
	}
	catch (...) {
		json element;

		element["SelectedSkin"] = "";

		config.merge_patch(element);
		std::ofstream cfgOutFile;
		cfgOutFile.open("O2GraphicExt.json");
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}
	char* playingOpiPath = (char*)FollowPointers(hOtwo, { 0x1C8884, 0x6E4 });
	if (!currentSkin.empty())
	{
		configPath = "Skins/" + currentSkin + "/O2GraphicExt.json";
		std::string customPlayingPath = "Skins\\" + currentSkin + "\\Playing.opi";
		if (customPlayingPath.size() > 255) customPlayingPath = (customPlayingPath.begin(), customPlayingPath.at(254));
		strcpy(playingOpiPath, customPlayingPath.c_str());
	}
	else
	{
		configPath = "O2GraphicExt.json";
		strcpy(playingOpiPath, "Image\\Playing1.opi");
	}

	cfgInFile.open(configPath);
	try {
		config = json::parse(cfgInFile);
	}
	catch (...) {
		std::ofstream cfgOutFile;
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
		config = json();
	}

	try {
		hitPosition = config["Noteplain"]["HitPosition"];
		laneWidth = config["Noteplain"]["LaneWidth"].get<std::vector<int>>();
		laneSpacing = config["Noteplain"]["LaneSpacing"];
		*noteThickness = config["Noteplain"]["ExtraNoteThickness"];
		lnTail = config["Noteplain"]["ShowLNTail"];
	}
	catch (...) {
		json element;
		for (int i = 0; i < 7; i++)
		{
			if (laneWidth.size() < 7) laneWidth.push_back(0);
			if (i == 6) {
				laneWidth[i] = *noteplainRectWidth - *noteOffset[i];
				break;
			}
			laneWidth[i] = *noteOffset[i + 1] - *noteOffset[i];
		}
		element["Noteplain"]["HitPosition"] = hitPosition;
		element["Noteplain"]["LaneWidth"] = laneWidth;
		element["Noteplain"]["LaneSpacing"] = laneSpacing;
		element["Noteplain"]["ExtraNoteThickness"] = *noteThickness;
		element["Noteplain"]["ShowLNTail"] = lnTail;

		config.merge_patch(element);
		std::ofstream cfgOutFile;
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}
	try {
		noteplainX = config["Noteplain"]["ColumnStart"];
		showMeasurelines = config["Noteplain"]["ShowMeasureLines"];
		showTargetBar = config["Noteplain"]["ShowTargetBar"];
	}
	catch (...) {
		json element;
		element["Noteplain"]["ColumnStart"] = noteplainX;
		element["Noteplain"]["ShowMeasureLines"] = showMeasurelines;
		element["Noteplain"]["ShowTargetBar"] = showTargetBar;

		config.merge_patch(element);
		std::ofstream cfgOutFile;
		cfgOutFile.open(configPath);
		cfgOutFile << std::setw(4) << config << std::endl;
		cfgOutFile.close();
	}
	cfgInFile.close();

	if (*lnTailOffset == 0x90909090)
	{
		*MessageBoxFunction = 0;
		strcpy(MessageBoxText, "Please re-enable LongNote Tail in EstrolMultiTool");
		*MessageBoxType = 1;
	}
	else *lnTailOffset = hitPosition - *noteThickness;
	*shortNoteOffset = *lnBodyOffset = *lnHeadLimit = *lnBodyLimit = hitPosition - *noteThickness;
	*guidelinesOffset = hitPosition - *noteThickness / 2;
	*noteplainRectHeight = *guideLineOffset = *measureLineOffset =  *noteRenderCutoff =
	*unkHitpos2 = *unkHitpos3 = hitPosition;

	if (hitPosition > 512) *playingRectHeight = hitPosition;

	*effectsY = 0x1DB + hitPosition - 480;

	std::vector<int> originalNoteWidth = { 28, 22, 28, 32, 28, 22, 28 };
	int totalWidth = 5;
	for (int i = 0; i < 7; i++)
	{
		if (noteWidth.size() < 7) noteWidth.push_back(0);
		noteWidth[i] = laneWidth[i] - originalNoteWidth[i];
		if (i == 0)
		{
			*effectOffset[i] = noteplainX - 21 + 5 + (originalNoteWidth[i] + noteWidth[i]) / 2;
			*noteOffset[i] = 5;
			totalWidth += laneWidth[i];
			continue;
		}
		*effectOffset[i] = *effectOffset[i - 1] + (laneWidth[i - 1] + laneWidth[i]) / 2 + laneSpacing;
		*noteOffset[i] = *noteOffset[i - 1] + laneWidth[i - 1] + laneSpacing;
		totalWidth += laneWidth[i] + laneSpacing;
	}
	*noteplainRectWidth = totalWidth;

	return InitPlayingScene(pThis, unk1, unk2);
}

typedef void(__thiscall* setRectPosition)(DWORD* pThis, DWORD arg1, DWORD posX, DWORD posY, DWORD arg4);
setRectPosition SetRectPosition = NULL; // Called from playingAni/setter.
setRectPosition SetRectPositionHidden = NULL;

void __fastcall OnSetRectPosition(DWORD* pThis, DWORD edx, DWORD arg1, DWORD posX, DWORD posY, DWORD arg4)
{
	if (*currentState == 11)
	{
		DWORD* rectanglesPtr = (DWORD*)FollowPointers(hOtwo, { 0x1C8884, 0x0D40, 0x620 });
		DWORD* noteplainRect = (DWORD*)((*rectanglesPtr) + 0x1148);
		DWORD* measurelineRect = (DWORD*)((*rectanglesPtr) + 0x1378);
		if (!showMeasurelines && pThis == measurelineRect) return;
		if (pThis == noteplainRect || pThis == measurelineRect)
		{
			posX = noteplainX;
		}
	}
	return SetRectPosition(pThis, arg1, posX, posY, arg4);
}

typedef void(__thiscall* renderNote)(DWORD* pThis, DWORD typePair, DWORD arg2, DWORD pos);
renderNote RenderNote = NULL; // Called from GameplayEvent.
std::vector<DWORD> noteTypes;
void __fastcall OnRenderNote(DWORD* pThis, DWORD edx, DWORD typePair, DWORD arg2, DWORD pos)
{
	if (!lnTail && ((char*)&typePair)[3] == 0x03) return;
	return RenderNote(pThis, typePair, arg2, pos);
}

typedef void(__thiscall* setNoteParams)(DWORD* pThis, DWORD arg1, DWORD columnIdx, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6, DWORD noteType, DWORD widthOffset, DWORD heightOffset);
setNoteParams SetNoteParams = NULL; // Called from RenderNote.

void __fastcall OnSetNoteParams(DWORD* pThis, DWORD edx, DWORD arg1, DWORD columnIdx, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6, DWORD noteType, DWORD widthOffset, DWORD heightOffset)
{
	return SetNoteParams(pThis, arg1, columnIdx, arg3, arg4, arg5, arg6, noteType, noteWidth[columnIdx], heightOffset);
}

typedef void(__thiscall* playDataAnimation)(Data* data, int playFor, int pauseFor, int arg3, int arg4);
playDataAnimation PlayDataAnimation = NULL;

typedef int(__thiscall* renderData)(Data* data, DWORD unk);
renderData RenderData = NULL;

typedef void(__thiscall* renderDataSimple)(Data* data, int frameIdx, int posX, int posY, DWORD* arg3, DWORD* arg4);
renderDataSimple RenderDataSimple = NULL;

void __fastcall OnRenderDataSimple(Data* data, DWORD edx, int frameIdx, int posX, int posY, DWORD* arg3, DWORD* arg4)
{
	if (data->type != 1) return RenderDataSimple(data, frameIdx, posX, posY, arg3, arg4);

	if (strncmp(data->frame->name, "Jam_gauge_bar.ojs", 17) == 0 ||
		strncmp(data->frame->name, "Jam_gauge_text.ojs", 18) == 0 ||
		strncmp(data->frame->name, "Note_BG.ojs", 11) == 0)
	{
		posX = noteplainX - 21;
	}
	if (strncmp(data->frame->name, "KeydownImage.ojs", 16) == 0 || strncmp(data->frame->name, "Keyeffect.ojs", 13) == 0)
	{
		int originalNoteOffset = 5;
		std::vector<int> originalNoteWidth = { 28, 22, 28, 32, 28, 22, 28 };
		for (int i = 1; i < frameIdx; i++)
		{
			originalNoteOffset += originalNoteWidth[i - 1];
		}
		posX = noteplainX - 21 + *noteOffset[frameIdx - 1] - originalNoteOffset;
	}
	if (strncmp(data->frame->name, "Keyeffect.ojs", 16) == 0 || strncmp(data->frame->name, "KeydownImage.ojs", 16) == 0 ||
		strncmp(data->frame->name, "Jam_gauge_bar.ojs", 17) == 0 || strncmp(data->frame->name, "Jam_gauge_text.ojs", 18) == 0)
	{
		posY = hitPosition - 480;
	}

	return RenderDataSimple(data, frameIdx, posX, posY, arg3, arg4);
}

typedef void(__thiscall* setEmbeddedPosition)(DWORD* pThis, DWORD posX, DWORD posY, DWORD arg3);
setEmbeddedPosition SetEmbeddedPosition = NULL;

void __fastcall OnSetEmbeddedPosition(DWORD* pThis, DWORD edx, DWORD posX, DWORD posY, DWORD arg3)
{
	if (*currentState == 11)
	{
		DWORD* currentStatePtr = (DWORD*)FollowPointers(hOtwo, { 0x1C8884, 0x0D40 });
		DWORD* hitTargetRect = (DWORD*)((*currentStatePtr) + 0x318);
		if (!showTargetBar && pThis == hitTargetRect) return;
		if (pThis == hitTargetRect)
		{
			posX = noteplainX + 1;
			posY = posY + hitPosition - 480;

			Resource* noteBgRes = (Resource*)FollowPointers(hOtwo, { 0x1C8884, 0x0D40,  0x598 + 0x4, 0 });
			RenderData(noteBgRes->data, 0);
		}
	}
	return SetEmbeddedPosition(pThis, posX, posY, arg3);
}

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

	/*if (!O2GraphicExt::addedResources.empty())
	{
		for (int i = 0; i < O2GraphicExt::addedResources.size(); i++)
		{
			RenderData(O2GraphicExt::addedResources[i]->data, 0);
		}
	}*/

	return result;
}

#include "GDIPlusManager.hpp"

#include <gdiplus.h>

#pragma comment(lib, "Gdiplus.lib")
struct RescaledBitmap {
	uint8_t* data;

	int width = 0;
	int height = 0;
	int frameCount = 0;
};

RescaledBitmap* RescaleImage24bit(int* pSourceResource, RescaledBitmap* pRescaledResource, int frameCount, int origWidth, int origHeight, int targetWidth, int targetHeight)
{
	GDIPlusManager::Init();
	RescaledBitmap* resPtr;

	{
		Gdiplus::Bitmap original(origWidth, origHeight * frameCount, PixelFormat24bppRGB);
		{
			for (int i = 0; i < origHeight * frameCount; i++) {
				for (int j = 0; j < origWidth; j++) {
					const char* pos = (char*)pSourceResource + j * 3 + i * (origWidth * 3);

					char r = pos[0];
					char g = pos[1];
					char b = pos[2];

					original.SetPixel(j, i, Gdiplus::Color(r, g, b));
				}
			}
		}

		Gdiplus::Bitmap bmp(targetWidth, targetHeight * frameCount, PixelFormat24bppRGB);
		{
			Gdiplus::Graphics graphics(&bmp);
			graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
			graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeNearestNeighbor);
			graphics.DrawImage(&original, 0, 0, targetWidth, targetHeight * frameCount);
		}

		RescaledBitmap& result = *pRescaledResource;
		result.width = targetWidth;
		result.height = targetHeight;
		result.frameCount = frameCount;
		result.data = new uint8_t[targetWidth * targetHeight * frameCount * 3];
		{
			int offset = 0;
			for (int y = 0; y < targetHeight * frameCount; y++) {
				for (int x = 0; x < targetWidth; x++) {
					Gdiplus::Color color;
					bmp.GetPixel(x, y, &color);

					result.data[offset] = color.GetR();
					result.data[offset + 1] = color.GetG();
					result.data[offset + 2] = color.GetB();

					offset += 3;
				}
			}
		}
		resPtr = &result;
	}

	GDIPlusManager::Deinit();
	return resPtr;
}

typedef int(__thiscall* O2BeforeLoadImage)(DWORD* pThis);
O2BeforeLoadImage BeforeLoadImage = nullptr;

typedef int(__thiscall* loadImage)(DWORD* pThis, int frame, int xRight, int yBottom, int* a5, int color);
loadImage pLoadImage = nullptr;
loadImage LoadLNImage = nullptr;

int __fastcall OnLoadLNImage(DWORD* pThis, DWORD edx, int frame, int xRight, int yBottom, int* a5, int color)
{
	return LoadLNImage(pThis, frame, xRight, hitPosition, a5, color);
}

typedef int(__thiscall* GameLoadImage)(DWORD* pThis, int* pResource);
GameLoadImage LoadTargetBar = nullptr;

RescaledBitmap targetBar = RescaledBitmap();
int __fastcall OnLoadTargetBar(DWORD* pThis, DWORD edx, int* pResource)
{
	BeforeLoadImage(pThis);
	RescaledBitmap& newRes = *RescaleImage24bit(pResource, &targetBar, 3, 190,73, 190 + *noteplainRectWidth - 193, 73);
	return pLoadImage(pThis, newRes.frameCount, newRes.width, newRes.height, (int*)newRes.data, -1);
}

GameLoadImage LoadMeasureLine = nullptr;

RescaledBitmap measureLine = RescaledBitmap();
int __fastcall OnLoadMeasureLine(DWORD* pThis, DWORD edx, int* pResource)
{
	BeforeLoadImage(pThis);
	RescaledBitmap& newRes = *RescaleImage24bit(pResource, &measureLine, 3, 0xBC, 3, 0xBC + *noteplainRectWidth - 193, 3);
	return pLoadImage(pThis, newRes.frameCount, newRes.width, newRes.height, (int*)newRes.data, -1);
}

void RescaleImage16bit(int* pSourceResource, int* pBuffer, int origWidth, int origHeight, int targetWidth, int targetHeight)
{
	GDIPlusManager::Init();
	{
		Gdiplus::Bitmap original(origWidth, origHeight, origWidth * 2, PixelFormat16bppRGB555, (BYTE*)pSourceResource);

		Gdiplus::Bitmap bmp(targetWidth, targetHeight, PixelFormat16bppRGB555);
		{
			Gdiplus::Graphics graphics(&bmp);
			graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
			graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeNearestNeighbor);
			graphics.DrawImage(&original, 0, 0, targetWidth, targetHeight);
		}

		Gdiplus::BitmapData data;
		Gdiplus::Rect rect(0, 0, bmp.GetWidth(), bmp.GetHeight());
		bmp.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat16bppRGB555, &data);
		for (int i = 0; i < bmp.GetHeight(); i++)
		{
			memcpy((void*)((uintptr_t)pBuffer + i * bmp.GetWidth() * 2), (void*)((uintptr_t)data.Scan0 + i * data.Stride), bmp.GetWidth() * 2);
		}
	}
	GDIPlusManager::Deinit();
	return;
}

typedef void(__thiscall* setSurfaceRect)(Frame* surface, int width, int height, int arg3, int alphaKey);
setSurfaceRect SetSurfaceRect = nullptr;

typedef int(__thiscall* readBitmapFromOpi)(Frame* surface, int* buffer, DWORD* vTable, short width, short height, short stride);
readBitmapFromOpi ReadBitmapFromOpi = nullptr;

int __fastcall OnReadBitmapFromOpi(Frame* surface, DWORD edx, int* buffer, DWORD* vTable, short width, short height, short stride)
{
	if (strncmp(surface->name, "Note_BG.ojs", 11) == 0)
	{
		ReadBitmapFromOpi(surface, buffer, vTable, width, height, stride);
		int newWidth = width + *noteplainRectWidth - 193;
		int* newBuffer = (int*)O2HeapAllocDirty(newWidth * height * 2);
		RescaleImage16bit(buffer, newBuffer, width, height, newWidth, height);
		O2HeapFree((uintptr_t*)buffer);
		BitmapDescription* currentBitmapDescription = (BitmapDescription*)((uintptr_t)surface->frameDescription + surface->currentIdx * sizeof(BitmapDescription));
		currentBitmapDescription->width = newWidth;
		currentBitmapDescription->imageSize = newWidth * height * 2;
		currentBitmapDescription->posX = 0;
		currentBitmapDescription->posY = 0;
		uintptr_t* currentBitmapPtr = (uintptr_t*)((uintptr_t)surface->bitmapPtr + 4 * surface->currentIdx);
		*currentBitmapPtr = (uintptr_t)newBuffer;
		SetSurfaceRect(surface, newWidth, height, 0, surface->alphaKey); // if size of this is less than surface->frameDescription sizes, the game will come to your place and break your knees. (if will fail to create DirectDrawSurface7 object)
		return 1;
	}
	if (strncmp(surface->name, "Keyeffect.ojs", 13) == 0)
	{
		ReadBitmapFromOpi(surface, buffer, vTable, width, height, stride);
		BitmapDescription* currentBitmapDescription = (BitmapDescription*)((uintptr_t)surface->frameDescription + surface->currentIdx * sizeof(BitmapDescription));
		int newWidth = laneWidth[surface->currentIdx];
		currentBitmapDescription->width = newWidth;
		currentBitmapDescription->imageSize = newWidth * height * 2;
		std::vector<int> defaultKeyeffectX = { 23, 51, 73, 101, 133, 161, 183 };
		currentBitmapDescription->posX = defaultKeyeffectX[surface->currentIdx];
		int* newBuffer = (int*)O2HeapAllocDirty(newWidth * height * 2);
		RescaleImage16bit(buffer, newBuffer, width, height, newWidth, height);
		O2HeapFree((uintptr_t*)buffer);
		uintptr_t* currentBitmapPtr = (uintptr_t*)((uintptr_t)surface->bitmapPtr + 4 * surface->currentIdx);
		*currentBitmapPtr = (uintptr_t)newBuffer;
		SetSurfaceRect(surface, newWidth, height, 0, surface->alphaKey);
		return 1;
	}
	if (strncmp(surface->name, "Note_Combo", 10) == 0 ||
		strncmp(surface->name, "Note_Cool", 9) == 0 ||
		strncmp(surface->name, "Note_Good", 9) == 0 ||
		strncmp(surface->name, "Note_Bad", 8) == 0 ||
		strncmp(surface->name, "Note_Miss", 9) == 0 ||
		strncmp(surface->name, "Note_Click", 10) == 0 ||
		strncmp(surface->name, "Note_Jam", 8) == 0)
	{
		BitmapDescription* currentBitmapDescription = (BitmapDescription*)((uintptr_t)surface->frameDescription + surface->currentIdx * sizeof(BitmapDescription));
		currentBitmapDescription->posX = 18;
		//SetSurfaceRect(surface, width, height, 0, surface->alphaKey);
	}

	return ReadBitmapFromOpi(surface, buffer, vTable, width, height, stride);
}

int O2GraphicExt::init(HMODULE hModule)
{
	hOtwo = (uintptr_t)GetModuleHandle("OTwo.exe");
	MODULEINFO modOtwo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hOtwo, &modOtwo, sizeof(MODULEINFO));

	O2HeapAllocClean = (o2HeapAlloc)((uintptr_t)hOtwo + 0xB9F8C);
	O2HeapAllocDirty = (o2HeapAlloc)((uintptr_t)hOtwo + 0xB7E62);
	O2HeapFree = (o2HeapFree)((uintptr_t)hOtwo + 0xA9DB0);

	LoadSceneElement = (loadSceneElement)((uintptr_t)hOtwo + 0x05B0C0);
	LoadRes = (loadRes)((uintptr_t)hOtwo + 0x0303A0);
	Sub_4690 = (sub_4690)((uintptr_t)hOtwo + 0x4690);
	InitPlayingSceneAdditional = (initPlayingSceneAdditional)((uintptr_t)hOtwo + 0x024B60);
	InitPlayingScene = (initPlayingScene)((uintptr_t)hOtwo + 0x03B870);
	SetEmbeddedPosition = (setEmbeddedPosition)((uintptr_t)hOtwo + 0x87280);
	SetRectPositionHidden = (setRectPosition)((uintptr_t)hOtwo + 0x0E9E0);
	SetRectPosition = (setRectPosition)((uintptr_t)hOtwo + 0x0EAC0);
	RenderNote = (renderNote)((uintptr_t)hOtwo + 0x07E9A0);
	SetNoteParams = (setNoteParams)((uintptr_t)hOtwo + 0x07D590);
	PlayDataAnimation = (playDataAnimation)((uintptr_t)hOtwo + 0x010DB0);
	RenderBg = (renderBg)((uintptr_t)hOtwo + 0x027D50);
	RenderPlaying = (renderPlaying)((uintptr_t)hOtwo + 0x027DD0);
	RenderData = (renderData)((uintptr_t)hOtwo + 0x010B90);
	RenderDataSimple = (renderDataSimple)((uintptr_t)hOtwo + 0x11440);
	ScaleOjt = (scaleOjt)((uintptr_t)hOtwo + 0x108C0);
	SetPosition = (setPosition)((uintptr_t)hOtwo + 0x113A0);
	DrawCombo = (drawCombo)((uintptr_t)hOtwo + 0x028780);
	DrawJamCombo = (drawJamCombo)((uintptr_t)hOtwo + 0x05C500);
	DrawPill = (drawPill)((uintptr_t)hOtwo + 0x5C6C0);

	SetSurfaceRect = (setSurfaceRect)((uintptr_t)hOtwo + 0x0B330);
	ReadBitmapFromOpi = (readBitmapFromOpi)((uintptr_t)hOtwo + 0x0E5A0);
	BeforeLoadImage = (O2BeforeLoadImage)((uintptr_t)hOtwo + 0x7D150);
	pLoadImage = (loadImage)((uintptr_t)hOtwo + 0x86CA0);
	LoadLNImage = (loadImage)((uintptr_t)hOtwo + 0x86E40);
	LoadTargetBar = (GameLoadImage)((uintptr_t)hOtwo + 0x7D0F0);
	LoadMeasureLine = (GameLoadImage)((uintptr_t)hOtwo + 0x7D030);

	playingRectHeight = (int*)((uintptr_t)hOtwo + 0x0DD03);
	noteplainRectWidth = (int*)((uintptr_t)hOtwo + 0x233E8);
	noteplainRectHeight = (int*)((uintptr_t)hOtwo + 0x233D1);
	shortNoteOffset = (int*)((uintptr_t)hOtwo + 0x7E33A);
	guideLineOffset = (int*)((uintptr_t)hOtwo + 0x7E3B3);
	measureLineOffset = (int*)((uintptr_t)hOtwo + 0x7E3C7);
	lnHeadLimit = (int*)((uintptr_t)hOtwo + 0x7E3D8);
	lnBodyLimit = (int*)((uintptr_t)hOtwo + 0x7E3F7);
	lnBodyOffset = (int*)((uintptr_t)hOtwo + 0x7E435);
	lnTailOffset = (int*)((uintptr_t)hOtwo + 0x7E467);
	noteRenderCutoff = (int*)((uintptr_t)hOtwo + 0x7EA2D);
	guidelinesOffset = (int*)((uintptr_t)hOtwo + 0x7E3DF);
	unkHitpos2 = (int*)((uintptr_t)hOtwo + 0x7E401);
	unkHitpos3 = (int*)((uintptr_t)hOtwo + 0x7E84E);
	noteThickness = (BYTE*)((uintptr_t)hOtwo + 0x7EA58);
	effectsY = (int*)((uintptr_t)hOtwo + 0x2557F);
	effectOffset[0] = (int*)((uintptr_t)hOtwo + 0x252EC);
	effectOffset[1] = (int*)((uintptr_t)hOtwo + 0x252F4);
	effectOffset[2] = (int*)((uintptr_t)hOtwo + 0x252FC);
	effectOffset[3] = (int*)((uintptr_t)hOtwo + 0x2530A);
	effectOffset[4] = (int*)((uintptr_t)hOtwo + 0x25314);
	effectOffset[5] = (int*)((uintptr_t)hOtwo + 0x2531C);
	effectOffset[6] = (int*)((uintptr_t)hOtwo + 0x25324);
	noteOffset[0] = (int*)((uintptr_t)hOtwo + 0x7EDB9);
	noteOffset[1] = (int*)((uintptr_t)hOtwo + 0x7EDC0);
	noteOffset[2] = (int*)((uintptr_t)hOtwo + 0x7EDC7);
	noteOffset[3] = (int*)((uintptr_t)hOtwo + 0x7EDCE);
	noteOffset[4] = (int*)((uintptr_t)hOtwo + 0x7EDD5);
	noteOffset[5] = (int*)((uintptr_t)hOtwo + 0x7EDDC);
	noteOffset[6] = (int*)((uintptr_t)hOtwo + 0x7EDE3);

	counterCoolX = (int*)((uintptr_t)hOtwo + 0x237EE);
	counterCoolY = (int*)((uintptr_t)hOtwo + 0x237E2);
	counterGoodX = (int*)((uintptr_t)hOtwo + 0x23816);
	counterGoodY = (int*)((uintptr_t)hOtwo + 0x2380B);
	counterBadX = (int*)((uintptr_t)hOtwo + 0x23834);
	counterBadY = (int*)((uintptr_t)hOtwo + 0x2382F);
	counterMissX = (int*)((uintptr_t)hOtwo + 0x23850);
	counterMissY = (int*)((uintptr_t)hOtwo + 0x23845);
	counterComboX = (int*)((uintptr_t)hOtwo + 0x23875);
	counterComboY = (int*)((uintptr_t)hOtwo + 0x23870);
	counterScoreX = (int*)((uintptr_t)hOtwo + 0x2378C);
	counterScoreY = (int*)((uintptr_t)hOtwo + 0x23787);
	indicatorVol1X = (int*)((uintptr_t)hOtwo + 0x23AC5);
	indicatorVol1Y = (int*)((uintptr_t)hOtwo + 0x23AB4);
	indicatorVol2X = (int*)((uintptr_t)hOtwo + 0x23B17);
	indicatorVol2Y = (int*)((uintptr_t)hOtwo + 0x23B06);
	indicatorNoteX = (int*)((uintptr_t)hOtwo + 0x7D22D);
	indicatorNoteY = (int*)((uintptr_t)hOtwo + 0x7D228);
	indicatorVisX = (int*)((uintptr_t)hOtwo + 0x241BB);
	indicatorVisY = (int*)((uintptr_t)hOtwo + 0x241B2);
	indicatorArrX = (int*)((uintptr_t)hOtwo + 0x241D3);
	indicatorArrY = (int*)((uintptr_t)hOtwo + 0x241CA);

	comboStartingY = (int*)((uintptr_t)hOtwo + 0x23379);
	comboTargetY = (int*)((uintptr_t)hOtwo + 0x27F18);
	comboIncrementY = (int8_t*)((uintptr_t)hOtwo + 0x27F3D);
	comboTextIncrementX = (int8_t*)((uintptr_t)hOtwo + 0x27F25);
	comboTextIncrementY = (int8_t*)((uintptr_t)hOtwo + 0x27F23);
	judgementInitialScale = (float*)((uintptr_t)hOtwo + 0x23348);
	judgementScaleCompounding = (float*)((uintptr_t)hOtwo + 0x27F83);

	uintptr_t* stateManager = (uintptr_t*)(hOtwo + 0x1C8884);
	while (!*stateManager) Sleep(50); // wait for StateManager to initialize.
	previousState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x4C });
	currentState = (int*)FollowPointers(hOtwo, { 0x1C8884, 0x50 });
	MessageBoxType = (short*)FollowPointers(hOtwo, { 0x1C8884, 0x5E });
	MessageBoxFunction = (short*)FollowPointers(hOtwo, { 0x1C8884, 0x5C });
	MessageBoxText = (char*)FollowPointers(hOtwo, { 0x1C8884, 0x64 });

	DWORD curProtection = 0;
	BOOL hResult = VirtualProtect((void*)(hOtwo + 0x2886A), 160, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x23348), 0x40, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x27F18), 0x80, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x23782), 0x100, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x23AB4), 0x100, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x241B2), 0x40, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7D228), 0x10, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x233E8), 0x4, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x252EC), 0x100, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x2557F), 0x4, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7EA58), 0x1, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7EDC0), 0x40, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x233D1), 0x4, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7E33A), 0x140, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7E84E), 0x4, PAGE_EXECUTE_READWRITE, &curProtection);
	hResult = VirtualProtect((void*)(hOtwo + 0x7EA2D), 0x4, PAGE_EXECUTE_READWRITE, &curProtection); 
	hResult = VirtualProtect((void*)(hOtwo + 0x0DD03), 0x4, PAGE_EXECUTE_READWRITE, &curProtection);

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

	if (MH_CreateHookEx((LPVOID)RenderDataSimple, &OnRenderDataSimple, &RenderDataSimple) != MH_OK)
	{
		Logger("Couldn't hook RenderDataSimple");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)DrawPill, &OnDrawPill, &DrawPill) != MH_OK)
	{
		Logger("Couldn't hook DrawPill");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)DrawJamCombo, &OnDrawJamCombo, &DrawJamCombo) != MH_OK)
	{
		Logger("Couldn't hook DrawJamCombo");
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

	if (MH_CreateHookEx((LPVOID)RenderPlaying, &OnRenderPlaying, &RenderPlaying) != MH_OK)
	{
		Logger("Couldn't hook PlayingLoop");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)InitPlayingScene, &OnInitPlayingScene, &InitPlayingScene) != MH_OK)
	{
		Logger("Couldn't hook InitPlayingScene");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)SetPosition, &OnSetPosition, &SetPosition) != MH_OK)
	{
		Logger("Couldn't hook SetPosition");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)SetEmbeddedPosition, &OnSetEmbeddedPosition, &SetEmbeddedPosition) != MH_OK)
	{
		Logger("Couldn't hook SetEmbeddedPosition");
		return 1;
	}

	//if (MH_CreateHookEx((LPVOID)SetRectPositionHidden, &OnSetRectPositionHidden, &SetRectPositionHidden) != MH_OK)
	//{
	//	Logger("Couldn't hook SetRectPositionHidden");
	//	return 1;
	//}

	if (MH_CreateHookEx((LPVOID)SetRectPosition, &OnSetRectPosition, &SetRectPosition) != MH_OK)
	{
		Logger("Couldn't hook SetRectPosition");
		return 1;
	}
	
	if (MH_CreateHookEx((LPVOID)RenderNote, &OnRenderNote, &RenderNote) != MH_OK)
	{
		Logger("Couldn't hook RenderNote");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)SetNoteParams, &OnSetNoteParams, &SetNoteParams) != MH_OK)
	{
		Logger("Couldn't hook SetNoteParams");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)ReadBitmapFromOpi, &OnReadBitmapFromOpi, &ReadBitmapFromOpi) != MH_OK)
	{
		Logger("Couldn't hook ReadBitmapFromOpi");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)LoadLNImage, &OnLoadLNImage, &LoadLNImage) != MH_OK)
	{
		Logger("Couldn't hook LoadLNImage");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)LoadTargetBar, &OnLoadTargetBar, &LoadTargetBar) != MH_OK)
	{
		Logger("Couldn't hook LoadTargetBar");
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)LoadMeasureLine, &OnLoadMeasureLine, &LoadMeasureLine) != MH_OK)
	{
		Logger("Couldn't hook LoadMeasureLine");
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

