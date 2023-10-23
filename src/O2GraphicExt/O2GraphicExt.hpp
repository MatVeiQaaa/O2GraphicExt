#pragma once

#include <windows.h>
#include <string>
#include <vector>

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

namespace O2GraphicExt
{
	inline std::vector<std::string> addResourcesOutside;
	inline std::vector<Resource*> addedResources;
	int init(HMODULE hModule);
	int exit();
}