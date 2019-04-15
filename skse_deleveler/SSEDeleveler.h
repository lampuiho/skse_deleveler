#pragma once

class BGSEncounterZone;
class TESActorBaseData;

typedef unsigned short(*GetEncounterZoneLevel)(BGSEncounterZone* zone);
typedef unsigned short(*GetScaledActorLevel)(TESActorBaseData* pActorData);

class SSEDeleveler
{
	uintptr_t baseAddress;
	uintptr_t moduleLength;

	int Hook(GetEncounterZoneLevel, GetScaledActorLevel);
	int FindSSEBase();

public:
	enum ErrorCode {
		errFindModule,
		errFindGetEncounterZoneLevel,
		errFindGetScaledActorLevel,
		errAllocateHooker,
		errHookGetEncounterZoneLevel,
		errHookGetScaledActorLevel,
		NumError
	};
	static const char* ErrorMessage[NumError];

	int Init();
	static unsigned short __fastcall GetEncounterZoneLevelHooked(BGSEncounterZone* zone);
	static unsigned short __fastcall GetScaledActorLevelHooked(TESActorBaseData *pActorData);
};
