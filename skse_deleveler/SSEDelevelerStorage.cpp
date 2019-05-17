#include <vector>
#include <tuple>
#include <common/IPrefix.h>
#include <skse64/GameReferences.h>
#include <skse64/Serialization.h>

#include "SSEDelevelerGlobal.h"
#include "SSEDeleveler.h"
#include "SSEDelevelerStorage.h"

#define VERSION 1
#define SAVEID 'SDLV'

int SSEDelevelerStorage::Init() {
	serialization->SetUniqueID(g_pluginHandle, SAVEID);

	serialization->SetRevertCallback(g_pluginHandle, Clear);
	serialization->SetSaveCallback(g_pluginHandle, Save);
	serialization->SetLoadCallback(g_pluginHandle, Load);

	return 0;
}

void SSEDelevelerStorage::AddUnique(TESForm* form, uint16_t lvl, uint16_t lvlo) {
	uniqActorLv.insert_or_assign(form->formID,std::make_tuple(form, lvl, lvlo));
}

void SSEDelevelerStorage::AddLock(TESForm* form, uint16_t lvl, uint16_t lvlo) {
	lockLv.insert_or_assign(form->formID, std::make_tuple(form, lvl, lvlo));
}

static bool CheckChar(TESForm* form) {
	bool result = form->formType == kFormType_NPC;
	if (!result) _ERROR("%x expected NPC type but got %x instead",form->formID, form->formType);
	return result;
}

static bool CheckLock(TESForm* form) {
	bool result = form->formType == kFormType_Reference;
	if (!result) _ERROR("%x expected ref type but got %x instead", form->formID, form->formType);
	return result;
}

void SSEDelevelerStorage::Clear(SKSESerializationInterface * intfc) {
	_MESSAGE("Clearing Unique");
	for (auto obj : gDelevelerStorageSingleton->uniqActorLv)	{
		TESForm* form = std::get<0>(obj.second);
		auto id = obj.first;
		if (id != form->formID) {
			_ERROR("%x has different id than one from pointer %x!", id, form->formID);
			form = gDelevelerSingleton.pLookupFormByID(id);
		}

		//if (!CheckChar(form)) continue;

		TESNPC* pBase;
		if (form->formType == kFormType_Character)
			pBase = (TESNPC*)((Actor*)form)->baseForm;
		else
			pBase = (TESNPC*)form;

		if (!pBase) {
			_MESSAGE("%x has no base form!", id);
			continue;
		}
		auto pActorData = &pBase->actorData;
		if (!pActorData) {
			_MESSAGE("%x of type %x has no actorData!", pBase->formID, pBase->formType);
			continue;
		}
		if ((pActorData->flags >> kFlag_Unique & 1) == 0)
			_ERROR("%x is not unique!", id);

		auto lvl = (UInt16)std::get<1>(obj.second);
		if (pActorData->level != lvl)
			_MESSAGE("%x's level was changed to %d from %d!", id, pActorData->level, lvl);

		auto lvlo = (UInt8)std::get<2>(obj.second);
		pActorData->flags |= (1UL << kFlag_PCLevelMult);
		pActorData->level = lvlo;
	}
	_MESSAGE("Clearing Lock");
	for (auto obj : gDelevelerStorageSingleton->lockLv) {
		TESForm* form = std::get<0>(obj.second);
		auto id = obj.first;
		if (id != form->formID) {
			_ERROR("%x has different id than one from pointer %x!", id, form->formID);
			form = gDelevelerSingleton.pLookupFormByID(id);
		}

		if (!CheckLock(form)) continue;
		auto pRef = (TESObjectREFR*)form;
		auto xloc = gDelevelerSingleton.pGetXLOCRef(pRef);
		if (xloc == 0)
			_MESSAGE("%x is not locked!", id);
		else {
			auto lvl = (UInt16)std::get<1>(obj.second);
			if (xloc->level != lvl)
				_MESSAGE("%x's level was changed to %d from %d!", id, xloc->level, lvl);
			
			auto lvlo = (UInt8)std::get<2>(obj.second);
			xloc->level = lvlo;
			xloc->flags |= (1UL << 2);
		}
	}

	gDelevelerStorageSingleton->uniqActorLv.clear();
	gDelevelerStorageSingleton->lockLv.clear();
}

#pragma pack(push, 1)
struct LvlSav {
	uint32_t id;
	uint16_t lv;
	LvlSav() {}
	LvlSav(uint32_t id, uint16_t lv) :id(id), lv(lv) {}
};
#pragma pack(pop)

void SSEDelevelerStorage::Save(SKSESerializationInterface *intfc) {
	if (!gDelevelerStorageSingleton->serialization->OpenRecord(kUnique, VERSION))
		_ERROR("Error opening unique data");

	std::vector<LvlSav> buf;
	buf.reserve(gDelevelerStorageSingleton->uniqActorLv.size());
	for (auto obj : gDelevelerStorageSingleton->uniqActorLv)
		buf.push_back(LvlSav(obj.first, std::get<1>(obj.second)));
	if (!intfc->WriteRecordData(buf.data(), buf.size()*sizeof(LvlSav)))
		_ERROR("Error writing unique data");


	if (!gDelevelerStorageSingleton->serialization->OpenRecord(kLock, VERSION))
		_ERROR("Error opening lock data");
	buf.clear();
	buf.reserve(gDelevelerStorageSingleton->lockLv.size());
	for (auto obj : gDelevelerStorageSingleton->lockLv)
		buf.push_back(LvlSav(obj.first, std::get<1>(obj.second)));
	if (!intfc->WriteRecordData(buf.data(), buf.size() * sizeof(LvlSav)))
		_ERROR("Error writing lock data");
}

static void loadUniqV1(SKSESerializationInterface *intfc, UInt32 length) {
	UInt32 size = length / sizeof(LvlSav);
	std::vector<LvlSav> buf(size);
	intfc->ReadRecordData(buf.data(), length);

	for (auto obj : buf) {
		TESForm* form = gDelevelerSingleton.pLookupFormByID(obj.id);
		if (!form) { _ERROR("%x returned null",obj.id); continue; }
		//if (!CheckChar(form)) continue;

		TESNPC* pBase;
		if (form->formType == kFormType_Character)
			pBase = (TESNPC*)((Actor*)form)->baseForm;
		else
			pBase = (TESNPC*)form;

		if (!pBase) {
			_ERROR("%x has no base form!", obj.id);
			continue;
		}
		auto pActorData = &pBase->actorData;
		if (!pActorData) {
			_ERROR("%x of type %x has no actorData!", pBase->formID, pBase->formType);
			continue;
		}

		if ((pActorData->flags >> kFlag_Unique & 1) == 0)
			_ERROR("%x is not unique!", obj.id);
		else {
			UInt16 lvlo = pActorData->level;
			pActorData->flags &= ~(1UL << kFlag_PCLevelMult);
			pActorData->level = obj.lv;
			gDelevelerStorageSingleton->AddUnique(pBase, obj.lv, lvlo);
		}
	}
}

static void loadLockV1(SKSESerializationInterface * intfc, UInt32 length) {
	UInt32 size = length / sizeof(LvlSav);
	std::vector<LvlSav> buf(size);
	intfc->ReadRecordData(buf.data(), length);

	for (auto obj : buf) {
		TESForm* form = gDelevelerSingleton.pLookupFormByID(obj.id);

		if (!CheckLock(form)) continue;

		auto pRef = (TESObjectREFR*)form;
		auto xloc = gDelevelerSingleton.pGetXLOCRef(pRef);
		if (xloc == 0)
			_ERROR("%x is not locked!", obj.id);
		else {
			UInt16 lvlo = xloc->level;
			xloc->flags &= ~(1UL << 2);
			xloc->level = obj.lv;
			gDelevelerStorageSingleton->AddLock(form, obj.lv, lvlo);
		}
	}
}

void SSEDelevelerStorage::LoadV1(SKSESerializationInterface *intfc, UInt32 type, UInt32 length) {
	switch (type) {
	case kUnique: {
		loadUniqV1(intfc, length);
		break;
	}
	case kLock: {
		loadLockV1(intfc, length);
		break;
	}
	}
}

void SSEDelevelerStorage::Load(SKSESerializationInterface * intfc) {
	_MESSAGE("Loading");
	UInt32	type;
	UInt32	version;
	UInt32	length;

	if (intfc->GetNextRecordInfo(&type, &version, &length)) {
		LoadV1(intfc, type, length);
	}
	if (intfc->GetNextRecordInfo(&type, &version, &length)) {
		LoadV1(intfc, type, length);
	}
}
