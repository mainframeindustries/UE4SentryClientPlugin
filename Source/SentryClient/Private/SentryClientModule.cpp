#include "SentryClientModule.h"
#include "SentryTransport.h"
#include "BlueprintLib.h"

#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformOutputDevices.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Kismet/KismetSystemLibrary.h"  // for user name
#include "HAL/PlatformProcess.h"	// for hostname


#include <stdio.h>


#define LOCTEXT_NAMESPACE "FSentryClientModule"

DEFINE_LOG_CATEGORY(LogSentryClient);
DEFINE_LOG_CATEGORY(LogSentryCore);

// Crash handling disableing was added in UE 5.1
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)	
#define HAVE_CRASH_HANDLING_THING 1
#else
#define HAVE_CRASH_HANDLING_THING 0
#endif

FSentryErrorOutputDevice FSentryClientModule::ErrorDevice = FSentryErrorOutputDevice();

void FSentryOutputDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{

	// do nothing if this is a too-high verbosity message
	if (Verbosity > module->GetVerbosity())
	{
		return;
	}
	StaticSerialize(V, Verbosity, Category);
	
}

void FSentryOutputDevice::StaticSerialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
#if SENTRY_HAVE_PLATFORM

	auto crumb = sentry_value_new_breadcrumb("debug", TCHAR_TO_UTF8(V));

	sentry_value_set_by_key(crumb, "category", sentry_value_new_string(TCHAR_TO_UTF8(*Category.ToString())));

	// find the level from verbosity
	const ANSICHAR* clevel = nullptr;
	switch (Verbosity)
	{
	case ELogVerbosity::Fatal:
		clevel = "fatal";
		break;
	case ELogVerbosity::Error:
		clevel = "error";
		break;
	case ELogVerbosity::Warning:
		clevel = "warning";
		break;
	case ELogVerbosity::Display:
	case ELogVerbosity::Log:
		clevel = "info";
		break;
	case ELogVerbosity::Verbose:
	case ELogVerbosity::VeryVerbose:
		clevel = "debug";
	}
	if (clevel != nullptr)
	{
		//todo, test using sentry_value_new_int
		sentry_value_set_by_key(crumb, "level", sentry_value_new_string(clevel));
	}

	sentry_add_breadcrumb(crumb);
#endif
}

// The ErrorOutputDevice is used by Assert handlers.  Use this to capture the assert message and send it to Sentry.
void FSentryErrorOutputDevice::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	// sent to our sentry handler, then pass on to previous.
	FSentryOutputDevice::StaticSerialize(V, ELogVerbosity::Fatal, Category);
	if (parent)
	{
		parent->Serialize(V, Verbosity, Category);
	}
}

void FSentryErrorOutputDevice::HandleError()
{
	if (parent)
	{
		parent->HandleError();
	}
}

#if SENTRY_HAVE_PLATFORM

static void _SentryLog(sentry_level_t level, const char* message, va_list args, void* userdata)
{
	FSentryClientModule* module = static_cast<FSentryClientModule*>(userdata);
	module->SentryLog((int)level, message, args);
}

static sentry_value_t _SentryCrash(const sentry_ucontext_t* uctx, sentry_value_t event, void* closure)
{
	FSentryClientModule* module = static_cast<FSentryClientModule*>(closure);
	return module->SentryCrash(uctx, event);
}

#endif


FString USentryClientConfig::GetEnvOrCmdLine(const TCHAR* name)
{
	FString out;
	if (GetEnvOrCmdLine(name, out))
	{
		return out;
	}
	return FString();
}

bool USentryClientConfig::GetEnvOrCmdLine(const TCHAR* name, FString& out)
{
	// check if there is a command line flag, of the format -SENTRY_name= or -SENTRY-name=
	FString cmd;
	FString key = FString::Printf(TEXT("SENTRY_%s"), name);
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-%s="), *key), cmd))
	{
		out = cmd;
		return true;
	}
	// try with hyphens instead of underscore
	FString altkey = key.Replace(TEXT("_"), TEXT("-"));
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-%s="), *altkey), cmd))
	{
		out = cmd;
		return true;
	}
	// empty string here is ambiguous, can mean not found, or set to empty string.
	// let's define empty env var as not exisiting.
	cmd = FPlatformMisc::GetEnvironmentVariable(*key);
	if (!cmd.IsEmpty())
	{
		// Trim whitespace from end.  This way, you can set an env var to whitespace, to mean 'empty' but existing
		cmd.TrimEndInline();
		out = cmd;
		return true;
	}
	return false;
}

FString USentryClientConfig::GetConfig(const TCHAR *name, const TCHAR *defaultval)
{
	FString result;
	if (GetEnvOrCmdLine(name, result))
	{
		return result;
	}
	return defaultval ? defaultval : TEXT("");
}

bool USentryClientConfig::IsEnabled()
{
	// command line or env can override
	FString val;
	if (GetEnvOrCmdLine(TEXT("ENABLED"), val))
	{
		if (val.IsEmpty() ||
			val == TEXT("0") ||
			val.Compare(TEXT("false"), ESearchCase::IgnoreCase) == 0 ||
			val.Compare(TEXT("no"), ESearchCase::IgnoreCase) == 0
			)
		{
			return false;
		}
		return true;
	}

	// Config can disable it
	bool enabled = Get()->Enabled;

	// and we can have logic to disable it too, e.g. for local builds.
	if (enabled)
	{
		enabled = !Get()->ShouldDisable();
	}
	return enabled;
}

// TODO: Add logic here for smart disabling of sentry, e.g. for local builds
bool USentryClientConfig::ShouldDisable()
{
	return false;
}

FString USentryClientConfig::GetDSN()
{
	return GetConfig(TEXT("DSN"), *Get()->DSN);
}

FString USentryClientConfig::GetEnvironment()
{
	return GetConfig(TEXT("ENVIRONMENT"), *Get()->Environment);
}

FString USentryClientConfig::GetRelease()
{
	return GetConfig(TEXT("RELEASE"), *Get()->Release);
}

FString USentryClientConfig::GetDatabasePath()
{
	return GetConfig(TEXT("DATABASE_PATH"), nullptr);
}

bool USentryClientConfig::IsConsentRequired()
{
	FString val = GetEnvOrCmdLine(TEXT("CONSENT_REQUIRED"));
	if (val.IsEmpty())
	{
		return Get()->ConsentRequired;
	}
	if (val.Equals(TEXT("yes"), ESearchCase::IgnoreCase) == 0)
		return true;
	if (val.Equals(TEXT("1"), ESearchCase::IgnoreCase) == 0)
		return true;
	if (val.Equals(TEXT("true"), ESearchCase::IgnoreCase) == 0)
		return true;
	if (val.Equals(TEXT("on"), ESearchCase::IgnoreCase) == 0)
		return true;
	return false;
}

TMap<FString, FString> USentryClientConfig::GetTags()
{
	FString val = GetConfig(TEXT("TAGS"), *Get()->Tags);
	
	// 1: split on commas, ignore empty values (allows trailing or leading commas)
	TArray<FString> tags;
	val.ParseIntoArray(tags, TEXT(","), true);

	// 2 construct the output mpa
	TMap<FString, FString> tagmap;
	for(auto &tag : tags)
	{
		FString key, value;
		if (tag.Split(TEXT("="), &key, &value))
		{
			tagmap.Emplace(key.TrimStartAndEnd(), value.TrimStartAndEnd());
		}
		else
		{
			tagmap.Emplace(tag.TrimStartAndEnd(), TEXT(""));
		}
	}
	return tagmap;
}


void FSentryClientModule::StartupModule()
{
#if SENTRY_HAVE_PLATFORM
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// plugin settings are generally accessed, in increased priority, from:
	// - Builtin
	// - Ini file
	// - Environment
	// - Command line

	LogDevice = MakeShareable(new FSentryOutputDevice(this));

	
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

	bool init = SentryInit(*dsn,
		env.IsEmpty() ? nullptr : *env,
		rel.IsEmpty() ? nullptr : *rel,
		USentryClientConfig::IsConsentRequired()
	);
	if (init)
	{
		// Set some default information (username, hostname)
		USentryBlueprintLibrary::SetUser(FString(), UKismetSystemLibrary::GetPlatformUserName(), FString());
		USentryBlueprintLibrary::SetTag(TEXT("hostname"), FPlatformProcess::ComputerName());

		// additional tags passed from command line
		auto tagmap = USentryClientConfig::GetTags();
		for(auto &elem : tagmap)
		{
			USentryBlueprintLibrary::SetTag(elem.Key, elem.Value);
		}
	}
#else
	UE_LOG(LogSentryClient, Log, TEXT("Module startup: dummy version, platform not supported"));
#endif
}

void FSentryClientModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	SentryClose();
}

bool FSentryClientModule::SentryInit(const TCHAR* DSN, const TCHAR* Environment, const TCHAR* Release, bool IsConsentRequired)
{
#if SENTRY_HAVE_PLATFORM
	if (initialized)
		SentryClose();
	check(DSN != nullptr);


	sentry_options_t* options = sentry_options_new();

	// Database location.  Pull in a config env var
	FString ConfigDBPath = USentryClientConfig::GetDatabasePath();
	if (!ConfigDBPath.IsEmpty())
	{
		dbPath = ConfigDBPath;
	}
	// Default database location in the game's Saved folder
	if (dbPath.IsEmpty())
	{
		dbPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("sentry-native"));
		dbPath = FPaths::ConvertRelativePathToFull(dbPath);
	}

#if PLATFORM_WINDOWS
	sentry_options_set_database_pathw(options, *dbPath);
#else
	sentry_options_set_database_path(options, TCHAR_TO_UTF8(*dbPath));
#endif

	// Location of the crashpad_backend.exe on windows
	// or crashpad_backend on linux.
	// This must match the SentryClient.build.cs paths
#if PLATFORM_WINDOWS
	if (CrashPadLocation.IsEmpty())
	{
		auto Plugin = IPluginManager::Get().FindPlugin(SENTRY_PLUGIN_NAME);
		FString BaseDir = Plugin->GetBaseDir();
		CrashPadLocation = FPaths::Combine(BaseDir,
			TEXT("Binaries/ThirdParty/sentry-native/Win64-Crashpad/bin/crashpad_handler.exe")
			);
		CrashPadLocation = FPaths::ConvertRelativePathToFull(CrashPadLocation);
	}
	sentry_options_set_handler_pathw(options, *CrashPadLocation);
#elif PLATFORM_LINUX
if (CrashPadLocation.IsEmpty())
	{
		auto Plugin = IPluginManager::Get().FindPlugin(SENTRY_PLUGIN_NAME);
		FString BaseDir = Plugin->GetBaseDir();
		CrashPadLocation = FPaths::Combine(BaseDir,
			TEXT("Binaries/ThirdParty/sentry-native/Linux/bin/crashpad_handler")
			);
		CrashPadLocation = FPaths::ConvertRelativePathToFull(CrashPadLocation);
	}
	sentry_options_set_handler_path(options, TCHAR_TO_UTF8(*CrashPadLocation));
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

	// We want sentry to send the log with any crash
	// TODO: make it possible to only add this with crashes, not other events?
	FString logfile = FPlatformOutputDevices::GetAbsoluteLogFilename();
	logfile = FPaths::ConvertRelativePathToFull(logfile);
#if PLATFORM_WINDOWS
	sentry_options_add_attachmentw(options, *logfile);
#else
	sentry_options_add_attachment(options, TCHAR_TO_UTF8(*logfile));
#endif

	// Consent handling
	sentry_options_set_require_user_consent(options, IsConsentRequired);

	// create a sentry transport
	sentry_options_set_transport(options, FSentryTransport::New());

	// Set an on crash callback
	sentry_options_set_on_crash(options, _SentryCrash, (void*)this);

	// Todo: set up a thing to add log breadcrumbs

	UE_LOG(LogSentryClient, Log, TEXT("Initializing with DSN '%s', env='%s', rel='%s'"), 
		DSN, Environment ? Environment : TEXT(""), Release ? Release : TEXT(""));
	int fail = sentry_init(options);
	if (!fail)
	{
		initialized = true;

		// Hook the log stream handler into GLog
		GLog->AddOutputDevice(LogDevice.Get());
		GLog->SerializeBacklog(LogDevice.Get());

		// Set the global error handler.  It is just a static object that we leave in place.
		if (GError != &ErrorDevice)
		{
			ErrorDevice.SetParent(GError);
			GError = &ErrorDevice;
		}

		// Set default context stuff
		SetupContext();

#if HAVE_CRASH_HANDLING_THING
		// instruct UE not to try to do crash handling itself
		FPlatformMisc::SetCrashHandlingType(ECrashHandlingType::Disabled);
#endif
	}
	else
	{
		UE_LOG(LogSentryClient, Warning, TEXT("Failed to initialize, code %d"), fail);
	}
	return initialized;
#else // SENTRY_HAVE_PLATFORM
	return false;
#endif
}

void FSentryClientModule::SentryClose()
{
#if SENTRY_HAVE_PLATFORM
	if (initialized)
	{
		if (GLog)
		{
			GLog->RemoveOutputDevice(LogDevice.Get());
		}

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
#endif
}

FSentryClientModule* FSentryClientModule::Get()
{
	auto * Module = FModuleManager::Get().GetModulePtr< FSentryClientModule>(TEXT("SentryClient"));
	return Module;
}

static bool crash_handled = false;

void FSentryClientModule::SentryLog(int level, const char* message, va_list args)
{
#if SENTRY_HAVE_PLATFORM

	ANSICHAR buf[512];
	auto formatted = FCStringAnsi::GetVarArgs(buf, sizeof(buf), message, args);
	const char* tlevel = "";
	switch ((sentry_level_t)level)
	{
	case SENTRY_LEVEL_DEBUG:
		UE_LOG(LogSentryCore, Verbose, TEXT("%s"), ANSI_TO_TCHAR(buf));
		tlevel = "debug";
		break;
	case SENTRY_LEVEL_INFO:
		UE_LOG(LogSentryCore, Log, TEXT("%s"), ANSI_TO_TCHAR(buf));
		tlevel = "info";
		break;
	case SENTRY_LEVEL_WARNING:
		UE_LOG(LogSentryCore, Warning, TEXT("%s"), ANSI_TO_TCHAR(buf));
		tlevel = "warning";
		break;
	case SENTRY_LEVEL_ERROR:
		UE_LOG(LogSentryCore, Error, TEXT("%s"), ANSI_TO_TCHAR(buf));
		tlevel = "error";
		break;
	case SENTRY_LEVEL_FATAL:
		// We cannot use the UE fatal level because it will terminate unreal.
		// fatal sentry errors can for example mean failure to start backend.
		// This is not fatal for unreal, though.
		UE_LOG(LogSentryCore, Error, TEXT("(sentry FATAL) %s"), ANSI_TO_TCHAR(buf));
		tlevel = "fatal";
		break;
	}

	// In case crash is being handled, the standard unreal logging system
	// has likely shut down.  Then we simply use stderr
	if (crash_handled)
	{
		if (GLog)
		{
			//Panic flush the logs to make sure there are no entries queued. This is
			//not thread safe so it will skip for example editor log.
# if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
			GLog->Panic();
# else
			GLog->PanicFlushThreadedLogs();
# endif
		}
		// also output to standard error, since the log subsystem may have crashed
		// if we are here during error handling
		FILE* out = stderr;
		fprintf(out, "Sentry [%s] : %s\n", tlevel, buf);
		fflush(out);
	}
#endif
}

sentry_value_t FSentryClientModule::SentryCrash(const sentry_ucontext_t* uctx, sentry_value_t event)
{
	// flag that a crash happened for the logger
	crash_handled = true;

#if 0
	// We can just defer to the Error handler if we want here, get the dialogue box and everything...
	// but that disables the sentry thing.
	if (GError)
	{
		GError->HandleError();
	}
#endif

	// Some code from WindowsCrashHandlingContext.cpp
	// 
	// Then try run time crash processing and broadcast information about a crash.
	FCoreDelegates::OnHandleSystemError.Broadcast();

	if (GLog)
	{
		//Panic flush the logs to make sure there are no entries queued. This is
		//not thread safe so it will skip for example editor log.
# if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
		GLog->Panic();
# else
		GLog->PanicFlushThreadedLogs();
# endif
	}
	return event;
}

void FSentryClientModule::SetupContext()
{
#if SENTRY_HAVE_PLATFORM

	// Add an "app" context, as described https://develop.sentry.dev/sdk/event-payloads/contexts/
	auto value = sentry_value_new_object();
	sentry_value_set_by_key(value, "type", sentry_value_new_string("app"));

	const TCHAR* Configuration = nullptr;
	auto conf = FApp::GetBuildConfiguration();
	if (conf == EBuildConfiguration::Debug)
		Configuration = TEXT("Debug");
	else if (conf == EBuildConfiguration::DebugGame)
		Configuration = TEXT("DebugGame");
	else if (conf == EBuildConfiguration::Development)
		Configuration = TEXT("Development");
	else if (conf == EBuildConfiguration::Shipping)
		Configuration = TEXT("Shipping");
	else if (conf == EBuildConfiguration::Test)
		Configuration = TEXT("Test");
	else
		Configuration = TEXT("Unknown");

	const TCHAR* TargetType = nullptr;
	auto target = FApp::GetBuildTargetType();
	if (target == EBuildTargetType::Game)
		TargetType = TEXT("Game");
	else if (target == EBuildTargetType::Server)
		TargetType = TEXT("Server");
	else if (target == EBuildTargetType::Client)
		TargetType = TEXT("Client");
	else if (target == EBuildTargetType::Editor)
		TargetType = TEXT("Editor");
	else if (target == EBuildTargetType::Program)
		TargetType = TEXT("Program");
	else
		TargetType = TEXT("Unknown");

	FString BuildType = FString::Printf(TEXT("%s-%s"), Configuration, TargetType);
	sentry_value_set_by_key(value, "build_type", sentry_value_new_string(TCHAR_TO_UTF8(*BuildType)));
	sentry_value_set_by_key(value, "build_date", sentry_value_new_string(TCHAR_TO_UTF8(*FApp::GetBuildDate())));
	
	FString Name = FApp::GetName();
	sentry_value_set_by_key(value, "app_name", sentry_value_new_string(TCHAR_TO_UTF8(*Name)));
	const TCHAR *Version = FApp::GetBuildVersion();
	sentry_value_set_by_key(value, "app_version", sentry_value_new_string(TCHAR_TO_UTF8(Version)));

	const TCHAR *CommandLine = FCommandLine::Get();
	sentry_value_set_by_key(value, "commandline", sentry_value_new_string(TCHAR_TO_UTF8(CommandLine)));
	sentry_set_context("app", value);

	// Set up an UnrealEngine context:
	value = sentry_value_new_object();
	sentry_value_set_by_key(value, "Target Type", sentry_value_new_string(TCHAR_TO_ANSI(TargetType)));
	sentry_value_set_by_key(value, "Configuration", sentry_value_new_string(TCHAR_TO_ANSI(Configuration)));
	sentry_value_set_by_key(value, "Product Identifier", sentry_value_new_string(TCHAR_TO_ANSI(*FApp::GetEpicProductIdentifier())));
	sentry_value_set_by_key(value, "Project Name", sentry_value_new_string(TCHAR_TO_ANSI(FApp::GetProjectName())));
	sentry_value_set_by_key(value, "Is Game", sentry_value_new_string(FApp::IsGame() ? "true" : "false"));
	sentry_value_set_by_key(value, "Is Engine Installed", sentry_value_new_string(FApp::IsEngineInstalled() ? "true" : "false"));
	sentry_set_context("unreal", value);

	// Set up some tags as well.
	sentry_set_tag("unreal.target_type", TCHAR_TO_ANSI(TargetType));
	sentry_set_tag("unreal.configuration", TCHAR_TO_ANSI(Configuration));
	sentry_set_tag("unreal.is_game", FApp::IsGame() ? "true" : "false");
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSentryClientModule, SentryClient)
