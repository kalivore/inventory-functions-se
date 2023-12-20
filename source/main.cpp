#include "skse64/PluginAPI.h"
#include "skse64_common/SafeWrite.h"
#include "skse64_common/skse_version.h"
#include <shlobj.h>

#include "InventoryFunctions64.h"

IDebugLog	gLog;
static bool initDone = false;

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

// here is a global reference to the interface, keeping to the skse style
SKSEPapyrusInterface        * g_papyrus = NULL;

// any other things the plugin does

extern "C"
{
	bool DoInit(const SKSEInterface* skse)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\" SAVE_FOLDER_NAME "\\SKSE\\InventoryFunctions.log");

		_MESSAGE("Inventory Functions SE Script");

		if (skse->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (skse->runtimeVersion < RUNTIME_VERSION_1_6_640)
		{
			_MESSAGE("unsupported runtime version");

			return false;
		}

		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
		if (!g_papyrus)
		{
			_MESSAGE("couldn't get papyrus interface");

			return false;
		}

		if (g_papyrus->interfaceVersion < SKSEPapyrusInterface::kInterfaceVersion)
		{
			_MESSAGE("papyrus interface too old (%d expected %d)", g_papyrus->interfaceVersion, SKSEPapyrusInterface::kInterfaceVersion);

			return false;
		}

		_MESSAGE("Init complete");

		initDone = true;

		return true;
	}

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Inventory Functions plugin";
		info->version = 1;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		bool initOk = DoInit(skse);

		return initOk;
	}

	__declspec(dllexport) SKSEPluginVersionData SKSEPlugin_Version =
	{
		SKSEPluginVersionData::kVersion,

		1,
		"Inventory Functions SE",

		"Kalivore",
		"support@example.com",

		0,	// not version independent (extended field)
		0,	// not version independent
		{ RUNTIME_VERSION_1_6_1130, 0 },	// compatible with 1.6.1130 only

		0,	// works with any version of the script extender. you probably do not need to put anything here
	};

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		bool initOk = false;
		if (!initDone)
		{
			initOk = DoInit(skse);
		}

		if (!initOk)
		{
			_MESSAGE("init Failed!");
			return false;
		}

		if (!g_papyrus)
		{
			_MESSAGE("g_papyrus Failed!");
			return false;
		}

		bool btest = g_papyrus->Register(InventoryFunctions_Papyrus::RegisterPapyrusFunctions);
		if (!btest)
		{
			_MESSAGE("g_papyrus Register Failed");
			return false;
		}

		_MESSAGE("Register Succeeded");

		return initOk;
	}

};
