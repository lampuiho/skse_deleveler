#include <common/IPrefix.h>
#include <shlobj.h>				// CSIDL_MYCODUMENTS
#include <skse64_common/skse_version.h>	// What version of SKSE is running?

#include "SSEDelevelerGlobal.h"
#include "SSEDelevelerStorage.h"
#include "SSEDeleveler.h"

const char* g_pluginName = "SSE Deleveler";
PluginHandle g_pluginHandle = kPluginHandle_Invalid;
SSEDelevelerStorage *gDelevelerStorageSingleton;

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
//#ifdef _DEBUG
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\skse_deleveler.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_DebugMessage);
		gLog.SetLogLevel(IDebugLog::kLevel_Error);

//		_MESSAGE(g_pluginName);
//#endif

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = g_pluginName;
		info->version = 0.02;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
		{
#ifdef _DEBUG
			_MESSAGE("loaded in editor, marking as incompatible");
#endif

			return false;
		}
		else if (skse->runtimeVersion < RUNTIME_VERSION_1_5_73)
		{
#ifdef _DEBUG
			_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);
#endif
			return false;
		}


		auto g_serialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
		if (!g_serialization)
		{
			_MESSAGE("couldn't get serialization interface");

			return false;
		}

		if (g_serialization->version < SKSESerializationInterface::kVersion)
		{
			_MESSAGE("serialization interface too old (%d expected %d)", g_serialization->version, SKSESerializationInterface::kVersion);

			return false;
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse) {	// Called by SKSE to load this plugin
		SSEDelevelerInit tmp;
		int result = tmp();
		if (result != 0)
		{/*
#ifndef _DEBUG
			gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\skse_deleveler.log");
			gLog.SetPrintLevel(IDebugLog::kLevel_Error);
			gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);
#endif*/
			_ERROR(SSEDelevelerInit::ErrorMessage[result]);
			return false;
		}

		gDelevelerStorageSingleton = new SSEDelevelerStorage((SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization));
		gDelevelerStorageSingleton->Init();

#ifdef _DEBUG
		 _MESSAGE(g_pluginName);
		 _MESSAGE(" loaded");
#endif
		return true;
	}
};
