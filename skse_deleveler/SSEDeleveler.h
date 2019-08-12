#pragma once

class TESForm;
class BGSEncounterZone;
class TESActorBaseData;
class Actor;
class TESObjectREFR;
struct ExtraLock;

typedef unsigned short(*GetEncounterZoneLevel)(BGSEncounterZone* zone);
typedef unsigned short(*GetActorLevel)(Actor*);
typedef unsigned short(*GetRefEncounterZoneLevel)(TESObjectREFR*,bool);
typedef ExtraLock*(*GetXLOCRef)(TESObjectREFR*);
typedef TESForm*(*_LookupFormByID)(UInt32);

enum kLockLevel {
	kLockEasy = 0,
	kLockAppre,
	kLockAdept,
	kLockExpert,
	kLockMaster,
	kLockKey
};

struct ExtraLock {
	UInt8 level;
	void* key;
	UInt8 flags;
};

struct SSEDeleveler {
	char *lockLevel[6];
	float *lockEncLvMult;
	GetRefEncounterZoneLevel pGetRefEncounterZoneLevel;
	GetXLOCRef pGetXLOCRef;
	_LookupFormByID pLookupFormByID;
};

class SSEDelevelerInit {
	uintptr_t baseAddress;
	uintptr_t moduleLength;
	GetEncounterZoneLevel pGetEncounterZoneLevel;
	GetActorLevel pGetActorLevel;
	uintptr_t pGetEZSavedLv;
	uintptr_t GetLvItemEncLv[3];
	uintptr_t GetPlEncLv;
	uintptr_t GetPlListLv[4];
	uintptr_t GetKeyLv[2];

	void ReadGlobalAddresses();
	int Hook();
	int FindSSEBase();
	static unsigned short GenSaveActorLvl(Actor*);

public:
	enum ErrorCode {
		errFindModule,
		errFindGetEncounterZoneLevel,
		errFindGetActorLevel,
		errFindGetEZSavedLv,
		errFindGetLvItemEncLv,
		errFindGetPlLv,
		errFindGetKeyLv,
		errFindLookUp,
		errAllocateHooker,
		errHookGetEncounterZoneLevel,
		errHookGetActorLevel,
		NumError
	};
	static const char* ErrorMessage[NumError];

	int operator()();
	static unsigned short __fastcall GetEncounterZoneLevelHooked(BGSEncounterZone* zone);
	static unsigned short __fastcall GetFluctuatedActorLevel(Actor *pRef);
	static unsigned short __fastcall GetActorLevelHooked(Actor *pRef);
	static int __fastcall GetKeyLevelHooked(ExtraLock*, TESObjectREFR*);
	static int __fastcall GetKeyDiffHooked(ExtraLock*, TESObjectREFR*);
};

extern SSEDeleveler gDelevelerSingleton;
