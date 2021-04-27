// Copyright Epic Games, Inc. All Rights Reserved.

#include "SentryClientModule.h"
#include "SentryTransport.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#define SENTRY_BUILD_STATIC 1 
#include "sentry.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"


#define LOCTEXT_NAMESPACE "FSentryClientModule"

DEFINE_LOG_CATEGORY(LogSentryClient);
DEFINE_LOG_CATEGORY(LogSentryCore);


#define HAVE_CRASH_HANDLING_THING 1

static void _SentryLog(sentry_level_t level, const char* message, va_list args, void* userdata)
{
	FSentryClientModule* module = static_cast<FSentryClientModule*>(userdata);
	module->SentryLog((int)level, message, args);
}


FString USentryClientConfig::GetEnvOrCmdLine(const TCHAR* name)
{
	// check if there is a command line flag, of the format -SENTRY_name=
	FString cmd;
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-SENTRY_%s="), name), cmd))
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

	

	FString dsn = USentryClientConfig::GetDSN();
	if (dsn.IsEmpty()) {
		UE_LOG(LogSentryClient, Log, TEXT("Module startup: No DSN specifiead, not initializing (.ini DSN, env/cmdline SENTRY_DSN"));
		return;
	}

	if (!USentryClientConfig::IsEnabled())
	{
		UE_LOG(LogSentryClient, Log, TEXT("Module startup: Disabled with .ini file (Enabled) or cmd line (SENTRY_ENABLED)"));
		return;
	}
	
	FString env = USentryClientConfig::GetEnvironment();
	FString rel = USentryClientConfig::GetRelease();

	SentryInit(*dsn,
		env.IsEmpty() ? nullptr : *env,
		rel.IsEmpty() ? nullptr : *rel
		);
}

void FSentryClientModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	SentryClose();
}

bool FSentryClientModule::SentryInit(const TCHAR* DSN, const TCHAR* Environment, const TCHAR* Release)
{
	if (initialized)
		SentryClose();
	check(DSN != nullptr);

	sentry_options_t* options = sentry_options_new();

	// Database location in the game's Saved folder
	if (dbPath.IsEmpty())
	{
		dbPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("sentry-native"));
		dbPath = FPaths::ConvertRelativePathToFull(dbPath);
	}

	sentry_options_set_database_pathw(options, *dbPath);

	// Location of the crashpad_backend.exe on windows
	// This must match the SentryClient.build.cs paths
#if PLATFORM_WINDOWS
	if (CrashPadLocation.IsEmpty())
	{
		auto Plugin = IPluginManager::Get().FindPlugin(SENTRY_PLUGIN_NAME);
		FString BaseDir = Plugin->GetBaseDir();
		CrashPadLocation = FPaths::Combine(BaseDir,
			TEXT("Source/ThirdParty/sentry-native/Win64/bin/crashpad_handler.exe")
			);
		CrashPadLocation = FPaths::ConvertRelativePathToFull(CrashPadLocation);
	}
	sentry_options_set_handler_pathw(options, *CrashPadLocation);
#endif

	sentry_options_set_dsn(options, TCHAR_TO_ANSI(DSN));
	if (Environment != nullptr)
	{
		sentry_options_set_environment(options, TCHAR_TO_ANSI(Environment));
	}
	if (Release != nullptr)
	{
		sentry_options_set_release(options, TCHAR_TO_ANSI(Release));
	}

	// setting debug means sentry will log to the provided logger
	sentry_options_set_logger(options, _SentryLog, (void*)this);
	sentry_options_set_debug(options, 1);

	// create a sentry transport
	sentry_options_set_transport(options, FSentryTransport::New());

	UE_LOG(LogSentryClient, Log, TEXT("Initializing with DSN '%s'"), 
		DSN, Environment ? Environment : TEXT(""), Release ? Release : TEXT(""));
	int fail = sentry_init(options);
	if (!fail)
	{
		initialized = true;

#if HAVE_CRASH_HANDLING_THING
		// instruct UE not to try to do crash handling itself
		FPlatformMisc::SetCrashHandling(ECrashHandlingType::Disabled);
#endif
	}
	else
	{
		UE_LOG(LogSentryClient, Warning, TEXT("Failed to initialize, code %d"), fail);
	}
	return initialized;
}

void FSentryClientModule::SentryClose()
{
	if (initialized)
	{
		int fail = sentry_close();
		if (!fail)
		{
			UE_LOG(LogSentryClient, Log, TEXT("Closed"));
		}
		else
		{ 
			UE_LOG(LogSentryClient, Error, TEXT("Failed to close, error %d"), fail);
		}
	}
	initialized = false;
}

FSentryClientModule* FSentryClientModule::Get()
{
	auto * Module = FModuleManager::Get().GetModulePtr< FSentryClientModule>(TEXT("SentryClient"));
	return Module;
}

void FSentryClientModule::SentryLog(int level, const char* message, va_list args)
{
	ANSICHAR buf[512];
	auto formatted = FCStringAnsi::GetVarArgs(buf, sizeof(buf), message, args);
	switch ((sentry_level_t)level)
	{
	case SENTRY_LEVEL_DEBUG:
		UE_LOG(LogSentryCore, Verbose, TEXT("%s"), ANSI_TO_TCHAR(buf));
		break;
	case SENTRY_LEVEL_INFO:
		UE_LOG(LogSentryCore, Display, TEXT("%s"), ANSI_TO_TCHAR(buf));
		break;
	case SENTRY_LEVEL_WARNING:
		UE_LOG(LogSentryCore, Warning, TEXT("%s"), ANSI_TO_TCHAR(buf));
		break;
	case SENTRY_LEVEL_ERROR:
		UE_LOG(LogSentryCore, Error, TEXT("%s"), ANSI_TO_TCHAR(buf));
		break;
	case SENTRY_LEVEL_FATAL:
		UE_LOG(LogSentryCore, Fatal, TEXT("%s"), ANSI_TO_TCHAR(buf));
		break;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSentryClientModule, SentryClient)