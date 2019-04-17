#include <common/IPrefix.h>
#include <skse64/GameReferences.h>

//#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"

#include "SSEDeleveler.h"
#include "MemUtil.h"
#include "Distribution.h"

#define AVGLevel 18

static const char* GetEncounterZoneLevelSig ="\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9"
	"\x48\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x44\x0F\xB7\xC0";
static const char* GetEncounterZoneLevelMask = "xxxxxxxxxxxx????x????xxxx";
static const char* GetScaledActorLevelSig = "\x8B\x41\x08\x0F\xB7\x51\x10";
static const char* GetScaledActorLevelMask = "xxxxxxx";
static const char* GetSavedEZLevelSig = "\x44\x0f\xb7\x40\x44"; //+0x2a6d9c
static const char* GetSavedEZLevelMask = "xxxxx";
//static const char* GetLevelMultiSig = "\x83\xf9\x03\x77\x00\x48\x63\xc1\x48\x8d\x0d\x00\x00\x00\x00\x48\x8b\x04\xc1\xf3";
//static const char* GetLevelMultiMask = "xxxx?xxxxxx????xxxxx";

const char* SSEDeleveler::ErrorMessage[SSEDeleveler::NumError] = { "errFindModule", "errFindGetEncounterZoneLevel", "errFindGetScaledActorLevel"
, "errAllocateHooker", "errHookGetEncounterZoneLevel", "errHookGetScaledActorLevel" };

int SSEDeleveler::Hook(GetEncounterZoneLevel pt1, GetScaledActorLevel pt2) {
	#pragma pack(push, 1)
	struct TrampolineCode {
		// jmp [rip]
		UInt8	escape;	// FF
		UInt8	modrm;	// 25
		UInt32	displ;	// 00000000
		// rip points here
		UInt64	dst;	// target

		TrampolineCode(uintptr_t _dst)	{
			escape = 0xFF;
			modrm = 0x25;
			displ = 0;
			dst = _dst;
		}
	};
	static_assert(sizeof(TrampolineCode) == 14,"Must be of size 14");

	TrampolineCode GetEncounterZoneLevelHookCode((uintptr_t)GetEncounterZoneLevelHooked);
	TrampolineCode GetScaledActorLevelHookCode((uintptr_t)GetScaledActorLevelHooked);
	SafeWriteBuf((uintptr_t)pt1, &GetEncounterZoneLevelHookCode, sizeof(TrampolineCode));
	SafeWriteBuf((uintptr_t)pt2, &GetScaledActorLevelHookCode, sizeof(TrampolineCode));
	

	//if (!g_branchTrampoline.Create(sizeof(TrampolineCode), NULL))
	//	return errAllocateHooker;
	//if (!g_branchTrampoline.Write5Branch((uintptr_t)pt1, (uintptr_t)GetEncounterZoneLevelHooked))
	//	return errHookGetEncounterZoneLevel;
	//if (!g_branchTrampoline.Write5Branch((uintptr_t)pt2, (uintptr_t)GetScaledActorLevelHooked))
	//	return errHookGetScaledActorLevel;

	//if (!SafeWriteJump((uintptr_t)pt1, (uintptr_t)GetEncounterZoneLevelHooked))
	//	return errHookGetEncounterZoneLevel;
	//if (!SafeWriteJump((uintptr_t)pt2, (uintptr_t)GetScaledActorLevelHooked))
	//	return errHookGetScaledActorLevel;
	return 0;
}

int SSEDeleveler::FindSSEBase() {
	auto process = GetCurrentProcess();
#ifdef _DEBUG
	char buf[30];
	_MESSAGE("processAddress");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)process);
	_MESSAGE(buf);
#endif

	auto handle = GetRemoteModuleHandle(process, "SkyrimSE.exe");
#ifdef _DEBUG
	_MESSAGE("handleAddress");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)handle);
	_MESSAGE(buf);
#endif

	GetModuleLen(process,handle,&baseAddress,&moduleLength);
#ifdef _DEBUG
	_MESSAGE("baseAddress");
	_snprintf_s(buf,sizeof(buf),"%016llx\n", baseAddress);
	_MESSAGE(buf);
	_MESSAGE("moduleLength");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", moduleLength);
	_MESSAGE(buf);
#endif

	if (baseAddress != 0 && moduleLength != 0)
		return 0;
	return 1;
}

int SSEDeleveler::Init() {
	if (FindSSEBase() > 0)
		return errFindModule;

	GetEncounterZoneLevel GetEncounterZoneLevelPtr = (GetEncounterZoneLevel)FindPattern((char*)this->baseAddress, GetEncounterZoneLevelSig
		, GetEncounterZoneLevelMask, moduleLength);
	if (GetEncounterZoneLevelPtr == 0)
		return errFindGetEncounterZoneLevel;

	GetScaledActorLevel GetScaledActorLevelPtr = (GetScaledActorLevel)FindPattern((char*)this->baseAddress, GetScaledActorLevelSig
			, GetScaledActorLevelMask, moduleLength);
	if (GetEncounterZoneLevelPtr == 0)
		return errFindGetScaledActorLevel;

#ifdef _DEBUG
	char buf[30];
	_MESSAGE("GetEncounterZoneLevelPtr");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)GetEncounterZoneLevelPtr);
	_MESSAGE(buf);
	_MESSAGE("GetScaledActorLevelPtr");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)GetScaledActorLevelPtr);
	_MESSAGE(buf);
#endif

	return Hook(GetEncounterZoneLevelPtr, GetScaledActorLevelPtr);
}

static float LevelFluctuator(float level) {
	return level + GenGaussRandFloat(0., 2);
}

static unsigned short CapFloatLevel(float level) {
	level = LevelFluctuator(level);
	level = max(1, std::roundf(level));
	level = min(SHRT_MAX, level);
	return (unsigned short)level;
}

static unsigned short GeneratePoisLevel(){
	float level = ComputePoisson(AVGLevel) + 1;
	return CapFloatLevel(level);
}

static unsigned short GenerateNormLevel(float maxL, float minL) {
	float mean = ComputeNormMean(maxL, minL);
	float stdd = (maxL - minL) / 6.f;
	auto level = GenGaussRandFloat(mean, stdd);
	return CapFloatLevel(level);
}

unsigned short SSEDeleveler::GetEncounterZoneLevelHooked(BGSEncounterZone* zone) {
	unsigned short result;

	byte minLevel = zone->minLevel;
	byte maxLevel = zone->maxLevel;
	result = GenerateNormLevel(maxLevel, minLevel);

	return result;
}

unsigned short SSEDeleveler::GetScaledActorLevelHooked(TESActorBaseData *pActorData) {
	//if (pActorData == &((TESNPC*)((*(g_thePlayer.GetPtr()))->baseForm))->actorData){
	//	return GeneratePoisLevel();
	//}
	auto result = pActorData->level;
	if ((pActorData->flags >> 7 & 1) != 0) {
		float mult = result * .001f; //to be used to multiply avg as skewed mean
		float minL = pActorData->minLevel;
		float maxL = pActorData->maxLevel;

		if (minL == 0 && maxL == 0)
			return CapFloatLevel((float)(ComputePoisson(mult *(float)AVGLevel) + 1));
		else if (minL == 0)
			minL = 1;
		if (maxL <= minL) {
			maxL = min(minL * exp((100. - minL) / 144.), 99);
		}

		result = GenerateNormLevel(maxL, minL);
		result *= mult;
	}
	return result;
}
