// Copyright Epic Games, Inc. All Rights Reserved.

#include "SentryClient.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#define SENTRY_BUILD_STATIC 1 
#include "sentry.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "GenericPlatform/GenericPlatformMisc.h"


#define LOCTEXT_NAMESPACE "FSentryClientModule"

DEFINE_LOG_CATEGORY(LogSentryClient);

FString USentryClientConfig::GetEnvOrCmdLine(const TCHAR* name)
{
	// check if there is a command line flag, of the format -SENTRY_name=
	FString cmd;
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-SENTRY_%s"), name), cmd))
		return cmd;
	return FGenericPlatformMisc::GetEnvironmentVariable(*FString::Printf(TEXT("SENTRY_%s"), name));
}

bool USentryClientConfig::IsEnabled()
{
	FString val = GetEnvOrCmdLine(TEXT("ENABLED"));
	if (!val.IsEmpty()) {
		if (val == TEXT("0") ||
			val.Compare(TEXT("false"), ESearchCase::IgnoreCase) == 0 ||
			val.Compare(TEXT("no"), ESearchCase::IgnoreCase) == 0
			)
		{
			return true;
		}
		return false;
	}
	return Get()->Enabled;
}

FString USentryClientConfig::GetDSN()
{
	FString val = GetEnvOrCmdLine(TEXT("DSN"));
	if (val.IsEmpty())
		val = Get()->DSN;
	return val;
}

FString USentryClientConfig::GetEnvironment()
{
	FString val = GetEnvOrCmdLine(TEXT("ENVIRONMENT"));
	if (val.IsEmpty())
		val = Get()->Environment;
	return val;
}

FString USentryClientConfig::GetRelease()
{
	FString val = GetEnvOrCmdLine(TEXT("RELEASE"));
	if (val.IsEmpty())
		val = Get()->Release;
	return val;
}

void FSentryClientModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// plugin settings are generally accessed, in increased priority, from:
	// - Builtin
	// - Ini file
	// - Environment
	// - Command line

	if (!USentryClientConfig::IsEnabled())
	{
		UE_LOG(LogSentryClient, Log, TEXT("SentryClient disabled with .ini file (Enabled) or cmd line (SENTRY_ENABLED)"));
		return;
	}

	FString dsn = USentryClientConfig::GetDSN();
	USentryClientConfig::Get()->DSN = "bar";
	USentryClientConfig::Get()->SaveConfig();
	if (dsn.IsEmpty()) {
		UE_LOG(LogSentryClient, Log, TEXT("No DSN specifiead, not initializing (.ini DSN, env/cmdline SENTRY_DSN"));
		return;
	}
	else
	{
		UE_LOG(LogSentryClient, Log, TEXT("Initializing with DSN '%s'"), *dsn);
	}


	// Database location in the game's Saved folder
	FString dbPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT(".sentry-native"));
	sentry_options_t* options = sentry_options_new();

	sentry_options_set_database_pathw(options, *dbPath);

	// Location of the crashpad_backend.exe on windows
#if PLATFORM_WINDOWS
	FString Plugins = FPaths::ProjectPluginsDir();
	FString PluginName = TEXT(SENTRY_PLUGIN_NAME);
	FString CrashPad = FPaths::Combine(Plugins, PluginName, TEXT("Source/ThirdParty/sentry-native/Win64/bin/crashpad_handler.exe"));
	sentry_options_set_handler_pathw(options, *CrashPad);
#endif


	sentry_options_set_dsn(options, "https://YOUR_KEY@oORG_ID.ingest.sentry.io/PROJECT_ID");
	sentry_init(options);
	initialized = true;
}

void FSentryClientModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (initialized)
	{
		sentry_close();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSentryClientModule, SentryClient)