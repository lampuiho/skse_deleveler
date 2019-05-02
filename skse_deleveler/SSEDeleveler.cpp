#include <common/IPrefix.h>
#include <skse64/GameReferences.h>

#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"

#include "SSEDeleveler.h"
#include "MemUtil.h"
#include "Distribution.h"

#define AVGLevel 18

//static const char* GetLevelMultiSig = "\x83\xf9\x03\x77\x00\x48\x63\xc1\x48\x8d\x0d\x00\x00\x00\x00\x48\x8b\x04\xc1\xf3";
//static const char* GetLevelMultiMask = "xxxx?xxxxxx????xxxxx";

static const char* GetEncounterZoneLevelSig ="\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9"
	"\x48\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x44\x0F\xB7\xC0";
static const char* GetEncounterZoneLevelMask = "xxxxxxxxxxxx????x????xxxx";
static const char* GetScaledActorLevelSig = "\x8B\x41\x08\x0F\xB7\x51\x10";
static const char* GetScaledActorLevelMask = "xxxxxxx";
static const char* GetEZSavedLvSig = "\x44\x0f\xb7\x40\x44"; //+0x2a6d9c
static const char* GetEZSavedLvMask = "xxxxx";
static const char* GetLevItemEnLvSig = "\xe8\x00\x00\x00\x00\xeb\x0c\x48\x85\xc9";
static const char* GetLevItemEnLvMask = "x????xxxxx";

static const char* GetKeyLvSig = "\x40\x53\x48\x83\xEC\x20\x0F\xB6\x19";//+1349D0, +134A90
static const char* GetKeyLvMask = "xxxxxxxxx";

static const char* GetPlLvSig1 = "\x48\x8b\xc8\xe8\x00\x00\x00\x00\xeb\x0c"; //+134a02,+134ac2
static const char* GetPlLvMask1 = "xxxx????xx";
static const size_t GetPlLvOff1 = 0x11;

static const char* GetPlLvSig2 = "\x48\x8b\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x44\x0f\xb7\xc0"; //+0x2a6db1
static const char* GetPlLvMask2 = "xxx????x????xxxx";
static const size_t GetPlLvOff2 = 0x7;

static const char* GetPlLvSig3 = "\xe8\x00\x00\x00\x00\x0f\xb7\xd0\x49\x8d\x4e\x30"; //+19cbdd
static const char* GetPlLvMask3 = "x????xxxxxxx";
static const char* GetPlLvSig4 = "\xe8\x00\x00\x00\x00\x0f\xb7\xd0\x48\x8d\x4f\x30"; //+377294
static const char* GetPlLvMask4 = "x????xxxxxxx";

static const char* GetPlLvSig5 = "\x0f\x5b\xf6\x44\x0f\x29\x44\x24\x60\xe8";
static const char* GetPlLvMask5 = "xxxxxxxxxx";
static const size_t GetPlLvOff5 = 0x9; //+19773b

static const char* GetPlLvSig6 = "\xe8\x00\x00\x00\x00\x0f\xb7\xc8\x3b\xcb"; //+197811
static const char* GetPlLvMask6 = "x????xxxxx";

SSEDeleveler sseDelevelerSingleton = SSEDeleveler();

static struct ExtraLock {
	UInt8 level;
	void* key;
	UInt8 flags;
};

static float LevelFluctuator(float level) {
	return level + GenGaussRandFloat(0., 2);
}

static unsigned short CapFloatLevel(float level) {
	level = LevelFluctuator(level);
	level = max(1, std::roundf(level));
	level = min(SHRT_MAX, level);
	return (unsigned short)level;
}

static unsigned short GeneratePoisLevel() {
	float level = ComputePoisson(AVGLevel) + 1;
	return CapFloatLevel(level);
}

static unsigned short GeneratePoisLevel(unsigned short minLevel){
	float level = ComputePoisson(AVGLevel + minLevel) + 1;
	return CapFloatLevel(level);
}

static unsigned short GenerateNormLevel(float maxL, float minL, float mult) {
	if (maxL == 0)
		return CapFloatLevel((float)(ComputePoisson(mult *(float)(AVGLevel + minL)) + 1));
	else if (minL == 0)
		minL = 1;
	if (maxL <= minL) {
		maxL = min(minL * exp((100. - minL) / 144.), 99);
	}
	float mean = ComputeNormMean(maxL, minL);
	float stdd = (maxL - minL) / 6.f;
	auto level = GenGaussRandFloat(mean, stdd);
	return CapFloatLevel(level);
}

static unsigned short GenerateNormLevel(float maxL, float minL) {
	if (maxL == 0)
		return CapFloatLevel((float)(ComputePoisson((float)(AVGLevel + minL)) + 1));
	else if (minL == 0)
		minL = 1;
	if (maxL <= minL) {
		maxL = min(minL * exp((100. - minL) / 144.), 99);
	}
	float mean = ComputeNormMean(maxL, minL);
	float stdd = (maxL - minL) / 6.f;
	auto level = GenGaussRandFloat(mean, stdd);
	return CapFloatLevel(level);
}

static unsigned short __fastcall GetEZSavedLevelHooked(BGSEncounterZone* zone) {
	unsigned short zoneLevel = zone->savedLevel;
	if (zoneLevel == 0)
		return SSEDelevelerInit::GetEncounterZoneLevelHooked(zone);
	else
		return CapFloatLevel(zoneLevel);
}

static void WriteNOP(uintptr_t start, byte nByte) {
	for (auto pos = start; pos < start + nByte; pos++)
		SafeWrite8(pos, 0x90);
}

static unsigned short GetRandLevel(TESActorBaseData *pActorData) {
	//ASSERT((pActorData->flags >> kFlag_PCLevelMult & 1) != 0);
	
	float mult = pActorData->level * .001f; //to be used to multiply avg as skewed mean
	float minL = pActorData->minLevel;
	float maxL = pActorData->maxLevel;

	auto result = GenerateNormLevel(maxL, minL, mult);
	result *= mult;

	if ((pActorData->flags >> kFlag_Unique & 1) != 0){
		pActorData->flags &= ~(1UL << kFlag_PCLevelMult);
		pActorData->level = result;
	}

	return result;
}

static unsigned short GetFluctuatedActorLevel(TESActorBaseData *pActorData) {
	if ((pActorData->flags >> kFlag_PCLevelMult & 1) != 0)
		return GetRandLevel(pActorData);
	else
		return CapFloatLevel(pActorData->level);
}

static unsigned short GetNonUniqueActorLevel(Actor* pRef) {
	return 1;
}

static unsigned short GetPlayerEncLevelHooked(Actor* pPlayer) {
	//if (pActorData == &((TESNPC*)((*(g_thePlayer.GetPtr()))->baseForm))->actorData){
	return GeneratePoisLevel();
	//}
}

static unsigned short GetPlayerItemLevelHooked(Actor* pPlayer) {
	return GenerateNormLevel(99, 1);
}

int SSEDelevelerInit::GetKeyLevelHooked(ExtraLock* lockData, TESObjectREFR* pRef) {
	int level = lockData->level;
	if ((level != *sseDelevelerSingleton.lockLevel[kLockKey]) && ((lockData->flags & 4) != 0)) {
		lockData->flags &= 0xFB;
		unsigned short encLv;
		if (!pRef) {
			encLv = GeneratePoisLevel();
		}
		else {
			encLv = sseDelevelerSingleton.pGetRefEncounterZoneLevel(pRef, false);
		}
		level += encLv * (int)(*sseDelevelerSingleton.lockEncLvMult);
		if (level > 99) {
			level = 99;
		}
		lockData->level = (byte)level;
	}
	return level;
}

int SSEDelevelerInit::GetKeyDiffHooked(ExtraLock* lockData, TESObjectREFR* pRef) {
	auto level = GetKeyLevelHooked(lockData, pRef);
	if (level <= *sseDelevelerSingleton.lockLevel[kLockEasy])
		return 0;
	if (level <= *sseDelevelerSingleton.lockLevel[kLockAppre])
		return 1;
	if (level <= *sseDelevelerSingleton.lockLevel[kLockAdept])
		return 2;
	if (level <= *sseDelevelerSingleton.lockLevel[kLockExpert])
		return 3;
	return (level > *sseDelevelerSingleton.lockLevel[kLockMaster]) + 4;
}

unsigned short SSEDelevelerInit::GetEncounterZoneLevelHooked(BGSEncounterZone* zone) {
	unsigned short result;

	byte minLevel = zone->minLevel;
	byte maxLevel = zone->maxLevel;
	result = GenerateNormLevel(maxLevel, minLevel);

	return result;
}

unsigned short SSEDelevelerInit::GetActorDataLevelHooked(TESActorBaseData *pActorData) {
	auto result = pActorData->level;
	if ((pActorData->flags >> kFlag_PCLevelMult & 1) != 0) {
		result = GetRandLevel(pActorData);
	}
	return result;
}

unsigned short SSEDelevelerInit::GetActorLevelHooked(Actor* pRef) {
	auto base = (TESNPC*)pRef->baseForm;
	if ((base->actorData.flags >> kFlag_PCLevelMult & 1) != 0)
		if ((base->actorData.flags >> kFlag_Unique & 1) != 0)
			return GetRandLevel(&base->actorData);
		else
			return GetNonUniqueActorLevel(pRef);
	else
		return base->actorData.level;
}

void SSEDelevelerInit::ReadGlobalAddresses() {
	sseDelevelerSingleton.lockLevel[kLockKey] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0xe)) + GetKeyLv[0] + 0x12);
	sseDelevelerSingleton.lockEncLvMult = (float*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x3e)) + GetKeyLv[0] + 0x42);
	sseDelevelerSingleton.lockLevel[kLockEasy] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x53)) + GetKeyLv[0] + 0x57);
	sseDelevelerSingleton.lockLevel[kLockAppre] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x63)) + GetKeyLv[0] + 0x67);
	sseDelevelerSingleton.lockLevel[kLockAdept] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x76)) + GetKeyLv[0] + 0x7a);
	sseDelevelerSingleton.lockLevel[kLockExpert] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x89)) + GetKeyLv[0] + 0x8d);
	sseDelevelerSingleton.lockLevel[kLockMaster] = (char*)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x9e)) + GetKeyLv[0] + 0xa2);
	sseDelevelerSingleton.pGetRefEncounterZoneLevel = (GetRefEncounterZoneLevel)((uintptr_t)(*(unsigned int*)(GetKeyLv[0] + 0x25)) + GetKeyLv[0] + 0x29);
}

int SSEDelevelerInit::Hook() {
#pragma pack(push, 1)
	struct TrampolineCode {
		// jmp [rip]
		UInt8	escape;	// FF
		UInt8	modrm;	// 25
		UInt32	displ;	// 00000000
		// rip points here
		UInt64	dst;	// target

		TrampolineCode(uintptr_t _dst) {
			escape = 0xFF;
			modrm = 0x25;
			displ = 0;
			dst = _dst;
		}
	};
	static_assert(sizeof(TrampolineCode) == 14, "TrampolineCode Must be of size 14");

	struct TramCallCode {
		UInt8 mov = 0x48;
		UInt8 ecx = 0x8b;
		UInt8 eax = 0xc8;
		UInt8 movabs = 0x48;	// 48 b8
		UInt8 moveabs2 = 0xb8;
		// rip points here
		UInt64	dst;	// target
		UInt8	escape = 0xff;	// FF
		UInt8	modrm = 0xD0;	// D0
		UInt8 jmp = 0xeb;
		UInt8 shrt = 0x10;

		TramCallCode(uintptr_t _dst, UInt8 _shrt) : dst(_dst), shrt(_shrt) {	}
	};
	static_assert(sizeof(TramCallCode) == 17, "TramCallCode Must be of size 17");

	struct HookCode
	{
		// jmp disp32
		UInt8	op;		// E9 for jmp, E8 for call
		SInt32	displ;	// 

		HookCode(SInt32 _displ, UInt8 _op) : op(_op), displ(_displ) {}
	};
	static_assert(sizeof(HookCode) == 5, "HookCode Must be of size 5");
#pragma pack(pop)

	TrampolineCode GetEncounterZoneLevelHookCode((uintptr_t)GetEncounterZoneLevelHooked);
	SafeWriteBuf((uintptr_t)GetEncounterZoneLevelPtr, &GetEncounterZoneLevelHookCode, sizeof(TrampolineCode));

	TrampolineCode GetScaledActorLevelHookCode((uintptr_t)GetActorDataLevelHooked);
	SafeWriteBuf((uintptr_t)GetScaledActorLevelPtr, &GetScaledActorLevelHookCode, sizeof(TrampolineCode));

	TrampolineCode GetKeyDiffHookCode((uintptr_t)GetKeyDiffHooked);
	SafeWriteBuf(GetKeyLv[0], &GetKeyDiffHookCode, sizeof(TrampolineCode));

	TrampolineCode GetKeyLevelHookCode((uintptr_t)GetKeyLevelHooked);
	SafeWriteBuf(GetKeyLv[1], &GetKeyLevelHookCode, sizeof(TrampolineCode));

	TramCallCode GetEZSavedLevelHookCode((uintptr_t)GetEZSavedLevelHooked,(UInt8)0x10);
	SafeWriteBuf(GetEZSavedLvPtr, &GetEZSavedLevelHookCode, sizeof(TramCallCode));
	WriteNOP(GetEZSavedLvPtr + sizeof(TramCallCode), 4);

	if (!g_branchTrampoline.Create(3*sizeof(TrampolineCode), NULL))
		return errAllocateHooker;
	//GetLevItemEnLv
	TrampolineCode * trampolineCode = (TrampolineCode *)g_branchTrampoline.Allocate(sizeof(TrampolineCode));
	if (trampolineCode)
	{
		*trampolineCode = TrampolineCode((uintptr_t)GetFluctuatedActorLevel);

		for (size_t i = 0; i < sizeof(GetLevItemEnLv) / sizeof(uintptr_t); i++)	{
			
			uintptr_t	trampolineAddr = uintptr_t(trampolineCode);
			uintptr_t	nextInstr = GetLevItemEnLv[i] + sizeof(HookCode);
			ptrdiff_t	trampolineDispl = trampolineAddr - nextInstr;

			// should never fail because we're branching in to the trampoline
			ASSERT((trampolineDispl >= _I32_MIN) && (trampolineDispl <= _I32_MAX));

			HookCode hookCode(trampolineDispl, 0xE8);
			SafeWriteBuf(GetLevItemEnLv[i], &hookCode, sizeof(HookCode));
		}
	}
	else
		return errAllocateHooker;
	//GetPlLv
	trampolineCode = (TrampolineCode *)g_branchTrampoline.Allocate(sizeof(TrampolineCode));
	if (trampolineCode)
	{
		*trampolineCode = TrampolineCode((uintptr_t)GetPlayerEncLevelHooked);

		//for (size_t i = 0; i < sizeof(GetPlLv) / sizeof(uintptr_t); i++) {

			uintptr_t	trampolineAddr = uintptr_t(trampolineCode);
			uintptr_t	nextInstr = GetPlLv + sizeof(HookCode);
			ptrdiff_t	trampolineDispl = trampolineAddr - nextInstr;

			// should never fail because we're branching in to the trampoline
			ASSERT((trampolineDispl >= _I32_MIN) && (trampolineDispl <= _I32_MAX));

			HookCode hookCode(trampolineDispl, 0xE8);
			SafeWriteBuf(GetPlLv, &hookCode, sizeof(HookCode));
		//}
	}
	else
		return errAllocateHooker;
	//GetPlItemLv
	trampolineCode = (TrampolineCode *)g_branchTrampoline.Allocate(sizeof(TrampolineCode));
	if (trampolineCode)
	{
		*trampolineCode = TrampolineCode((uintptr_t)GetPlayerItemLevelHooked);

		for (size_t i = 0; i < sizeof(GetPlItemLv) / sizeof(uintptr_t); i++) {

			uintptr_t	trampolineAddr = uintptr_t(trampolineCode);
			uintptr_t	nextInstr = GetPlItemLv[i] + sizeof(HookCode);
			ptrdiff_t	trampolineDispl = trampolineAddr - nextInstr;

			// should never fail because we're branching in to the trampoline
			ASSERT((trampolineDispl >= _I32_MIN) && (trampolineDispl <= _I32_MAX));

			HookCode hookCode(trampolineDispl, 0xE8);
			SafeWriteBuf(GetPlItemLv[i], &hookCode, sizeof(HookCode));
		}
	}
	else
		return errAllocateHooker;

	//if (!SafeWriteJump((uintptr_t)pt1, (uintptr_t)GetEncounterZoneLevelHooked))
	//	return errHookGetEncounterZoneLevel;
	//if (!SafeWriteJump((uintptr_t)pt2, (uintptr_t)GetScaledActorLevelHooked))
	//	return errHookGetScaledActorLevel;
	return 0;
}

int SSEDelevelerInit::FindSSEBase() {
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

	GetModuleLen(process, handle, &baseAddress, &moduleLength);
#ifdef _DEBUG
	_MESSAGE("baseAddress");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", baseAddress);
	_MESSAGE(buf);
	_MESSAGE("moduleLength");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", moduleLength);
	_MESSAGE(buf);
#endif

	if (baseAddress != 0 && moduleLength != 0)
		return 0;
	return 1;
}

int SSEDelevelerInit::operator()() {
	if (FindSSEBase() > 0)
		return errFindModule;

	GetEncounterZoneLevelPtr = (GetEncounterZoneLevel)FindPattern((char*)this->baseAddress, GetEncounterZoneLevelSig
		, GetEncounterZoneLevelMask, moduleLength);
	if (GetEncounterZoneLevelPtr == 0)
		return errFindGetEncounterZoneLevel;

	GetScaledActorLevelPtr = (GetScaledActorLevel)FindPattern((char*)this->baseAddress, GetScaledActorLevelSig
		, GetScaledActorLevelMask, moduleLength);
	if (GetScaledActorLevelPtr == 0)
		return errFindGetScaledActorLevel;

	GetEZSavedLvPtr = (uintptr_t)FindPattern((char*)this->baseAddress, GetEZSavedLvSig, GetEZSavedLvMask, moduleLength);
	if (GetEZSavedLvPtr == 0)
		return errFindGetEZSavedLv;

	auto scanModuleLength = moduleLength;
	auto startAddress = this->baseAddress;
	for (size_t i = 0; i < sizeof(GetLevItemEnLv) / sizeof(uintptr_t); i++) {
		GetLevItemEnLv[i] = (uintptr_t)FindPattern((char*)startAddress, GetLevItemEnLvSig, GetLevItemEnLvMask, scanModuleLength);
		if (GetLevItemEnLv[i] == 0)
			return errFindGetLevItemEnLv;
		scanModuleLength = scanModuleLength + startAddress - GetLevItemEnLv[i] - 10;
		startAddress = GetLevItemEnLv[i]+10;
	}

	/*
	scanModuleLength = moduleLength;
	startAddress = this->baseAddress;
	for (size_t i = 0; i < 2; i++) {
		GetPlLv[i] = (uintptr_t)FindPattern((char*)startAddress, GetPlLvSig1, GetPlLvMask1, scanModuleLength);
		if (GetPlLv[i] == 0)
			return errFindGetLevItemEnLv;
		GetPlLv[i] += GetPlLvOff1;
		scanModuleLength = scanModuleLength + startAddress - GetPlLv[i];
		startAddress = GetPlLv[i];
	}*/

	GetPlLv = (uintptr_t)FindPattern((char*)GetEncounterZoneLevelPtr+50, GetPlLvSig2, GetPlLvMask2
		, moduleLength+this->baseAddress-(uintptr_t)GetEncounterZoneLevelPtr-50);
	if (GetPlLv == 0)
		return errFindGetPlLv;
	GetPlLv += GetPlLvOff2;

	GetPlItemLv[0] = (uintptr_t)FindPattern((char*)this->baseAddress, GetPlLvSig3, GetPlLvMask3, moduleLength);
	GetPlItemLv[1] = (uintptr_t)FindPattern((char*)GetPlItemLv[0] + 50, GetPlLvSig4, GetPlLvMask4
		, moduleLength + this->baseAddress - GetPlItemLv[0] - 50);
	GetPlItemLv[2] = (uintptr_t)FindPattern((char*)this->baseAddress, GetPlLvSig5, GetPlLvMask5, moduleLength);
	GetPlItemLv[2] += GetPlLvOff5;
	GetPlItemLv[3] = (uintptr_t)FindPattern((char*)GetPlItemLv[2], GetPlLvSig6, GetPlLvMask6, moduleLength);

	
	scanModuleLength = moduleLength;
	startAddress = this->baseAddress;
	for (size_t i = 0; i < 2; i++) {
		GetKeyLv[i] = (uintptr_t)FindPattern((char*)startAddress, GetKeyLvSig, GetKeyLvMask, scanModuleLength);
		if (GetKeyLv[i] == 0)
			return errFindGetKeyLv;
		scanModuleLength = scanModuleLength + startAddress - GetKeyLv[i] - 10;
		startAddress = GetKeyLv[i] + 10;
	}

#ifdef _DEBUG
	char buf[30];
	_MESSAGE("GetEncounterZoneLevelPtr");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)GetEncounterZoneLevelPtr);
	_MESSAGE(buf);
	_MESSAGE("GetScaledActorLevelPtr");
	_snprintf_s(buf, sizeof(buf), "%016llx\n", (uintptr_t)GetScaledActorLevelPtr);
	_MESSAGE(buf);
#endif

	ReadGlobalAddresses();

	return Hook();
}

const char* SSEDelevelerInit::ErrorMessage[SSEDelevelerInit::NumError] = { "errFindModule", "errFindGetEncounterZoneLevel", "errFindGetScaledActorLevel",
"errFindGetEZSavedLv", "errFindGetLevItemEnLv", "errFindGetPlLv", "errFindGetKeyLv",
"errAllocateHooker", "errHookGetEncounterZoneLevel", "errHookGetScaledActorLevel" };
