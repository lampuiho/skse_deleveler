#pragma once

class BGSEncounterZone;
class TESActorBaseData;
class Actor;
class TESObjectREFR;
struct ExtraLock;

typedef unsigned short(*GetEncounterZoneLevel)(BGSEncounterZone* zone);
typedef unsigned short(*GetScaledActorLevel)(TESActorBaseData* pActorData);
typedef unsigned short(*GetRefEncounterZoneLevel)(TESObjectREFR*,bool);

enum kLockLevel {
	kLockEasy = 0,
	kLockAppre,
	kLockAdept,
	kLockExpert,
	kLockMaster,
	kLockKey
};

struct SSEDeleveler {
	char *lockLevel[6];
	float *lockEncLvMult;
	GetRefEncounterZoneLevel pGetRefEncounterZoneLevel;
};

class SSEDelevelerInit {
	uintptr_t baseAddress;
	uintptr_t moduleLength;
	GetEncounterZoneLevel GetEncounterZoneLevelPtr;
	GetScaledActorLevel GetScaledActorLevelPtr;
	uintptr_t GetEZSavedLvPtr;
	uintptr_t GetLevItemEnLv[3];
	uintptr_t GetPlLv;
	uintptr_t GetPlItemLv[4];
	uintptr_t GetKeyLv[2];

	void ReadGlobalAddresses();
	int Hook();
	int FindSSEBase();

public:
	enum ErrorCode {
		errFindModule,
		errFindGetEncounterZoneLevel,
		errFindGetScaledActorLevel,
		errFindGetEZSavedLv,
		errFindGetLevItemEnLv,
		errFindGetPlLv,
		errFindGetKeyLv,
		errAllocateHooker,
		errHookGetEncounterZoneLevel,
		errHookGetScaledActorLevel,
		NumError
	};
	static const char* ErrorMessage[NumError];

	int operator()();
	static unsigned short __fastcall GetEncounterZoneLevelHooked(BGSEncounterZone* zone);
	static unsigned short __fastcall GetActorDataLevelHooked(TESActorBaseData *pActorData);
	static unsigned short __fastcall GetActorLevelHooked(Actor *pRef);
	static int __fastcall GetKeyLevelHooked(ExtraLock*, TESObjectREFR*);
	static int __fastcall GetKeyDiffHooked(ExtraLock*, TESObjectREFR*);
};

extern SSEDeleveler sseDelevelerSingleton;
