#pragma once

#include <utility>
#include <map>

class TESForm;
struct SKSESerializationInterface;

class SSEDelevelerStorage {
	enum {
		kUnique,
		kLock,
		kRespawn
	};
	// ID, current level, original level
	std::map<uint32_t,std::tuple<TESForm*,uint16_t, uint16_t>> uniqActorLv;
	std::map<uint32_t, std::tuple<TESForm*, uint16_t, uint16_t>> lockLv;
	SKSESerializationInterface*	serialization;
	static void Clear(SKSESerializationInterface *intfc);
	static void Save(SKSESerializationInterface *intfc);
	static void Load(SKSESerializationInterface *intfc);
	static void LoadV1(SKSESerializationInterface*, UInt32, UInt32);
	
public:
	SSEDelevelerStorage(SKSESerializationInterface* serialization) : serialization(serialization) {}
	int Init();
	void AddUnique(TESForm*, uint16_t, uint16_t);
	void AddLock(TESForm*, uint16_t, uint16_t);
};

extern SSEDelevelerStorage *gDelevelerStorageSingleton;