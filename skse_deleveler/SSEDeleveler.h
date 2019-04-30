#pragma once

class BGSEncounterZone;
class TESActorBaseData;
class Actor;

typedef unsigned short(*GetEncounterZoneLevel)(BGSEncounterZone* zone);
typedef unsigned short(*GetScaledActorLevel)(TESActorBaseData* pActorData);

class SSEDeleveler
{
	uintptr_t baseAddress;
	uintptr_t moduleLength;
	GetEncounterZoneLevel GetEncounterZoneLevelPtr;
	GetScaledActorLevel GetScaledActorLevelPtr;
	uintptr_t GetEZSavedLvPtr;
	uintptr_t GetLevItemEnLv[3];
	uintptr_t GetPlLv[3];
	uintptr_t GetPlItemLv[4];

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
		errAllocateHooker,
		errHookGetEncounterZoneLevel,
		errHookGetScaledActorLevel,
		NumError
	};
	static const char* ErrorMessage[NumError];

	int Init();
	static unsigned short __fastcall GetEncounterZoneLevelHooked(BGSEncounterZone* zone);
	static unsigned short __fastcall GetActorDataLevelHooked(TESActorBaseData *pActorData);
	static unsigned short __fastcall GetActorLevelHooked(Actor *pRef);
};
