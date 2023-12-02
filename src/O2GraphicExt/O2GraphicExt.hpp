#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct ResDetails
{
	DWORD idc;
	char name[256];
};

struct BitmapDescription
{
	short posX;
	short posY;
	unsigned short width;
	unsigned short height;
	unsigned short offset;
	short padding1;
	unsigned short imageSize;
	short padding2;
	int padding3;
};

struct Frame
{
	uintptr_t* vTable;
	BYTE unk1[0x14];
	char name[256];
	short unk2;
	short encoder;
	short frameCount;
	short alphaKey;
	BitmapDescription* frameDescription;
	uintptr_t* bitmapPtr;
	BYTE unk3[0xC];
	unsigned int currentIdx;
	BYTE unk4[0x8];
	int YSize;
	int XSize;
};

struct OJT
{
	uintptr_t* vTable;
	BYTE unk1[0x24];
	Frame* frame;
	float XScale;
	float YScale;
	BYTE unk2[0x20];
	int currentFrame;
	BYTE unk3[0x4];
	float scaleAllRelative;
};

struct Data
{
	uintptr_t* vTable;
	Frame* frame;
	BYTE unk1[0x4];
	OJT* ojt;
	BYTE unk2[0x4];
	short type; // 1 == ojs, 3 == ojt
	short unk3; // Using it as "bool Looping" for my animation system, unknown what it is for the game.
	int X;
	int Y;
	BYTE unk4[0x120];
	int animationDuration; // Default = 30.
	int animationStart; // ?
	int animationCutoff; // Using this to store animationDuration original value for my animation system.
	BYTE unk5[0x18];
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