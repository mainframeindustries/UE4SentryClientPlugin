#include "SentryClientModule.h"
#include "SentryTransport.h"



#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformOutputDevices.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Kismet/KismetSystemLibrary.h"  // for user name
#include "GenericPlatform/GenericPlatformProcess.h"	// for hostname

#include "BlueprintLib.h"


#define LOCTEXT_NAMESPACE "FSentryClientModule"

DEFINE_LOG_CATEGORY(LogSentryClient);
DEFINE_LOG_CATEGORY(LogSentryCore);

// Define this as 1 if your UnrealEngine can control crash handler settings.
#define HAVE_CRASH_HANDLING_THING 0

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


static void _SentryLog(sentry_level_t level, const char* message, va_list args, void* userdata)
{
	FSentryClientModule* module = static_cast<FSentryClientModule*>(userdata);
	module->SentryLog((int)level, message, args);
}

static sentry_value_t SentryEventFunction(sentry_value_t event, void* hint, void* closure)
{
	// Some code from WindowsCrashHandlingContext.cpp
	// 
	// Then try run time crash processing and broadcast information about a crash.
	FCoreDelegates::OnHandleSystemError.Broadcast();

	if (GLog)
	{
		//Panic flush the logs to make sure there are no entries queued. This is
		//not thread safe so it will skip for example editor log.
		GLog->PanicFlushThreadedLogs();
	}
	return event;
}

FString USentryClientConfig::GetEnvOrCmdLine(const TCHAR* name)
{
	// check if there is a command line flag, of the format -SENTRY_name= or -SENTRY-name=
	FString cmd;
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-SENTRY_%s="), name), cmd))
	{
		return cmd;
	}
	if (FParse::Value(FCommandLine::Get(), *FString::Printf(TEXT("-SENTRY-%s="), name), cmd))
	{
		return cmd;
	}
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


void FSentryClientModule::StartupModule()
{
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
	}
}

void FSentryClientModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	SentryClose();
}

bool FSentryClientModule::SentryInit(const TCHAR* DSN, const TCHAR* Environment, const TCHAR* Release, bool IsConsentRequired)
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

#if PLATFORM_WINDOWS
	sentry_options_set_database_pathw(options, *dbPath);
#else
	sentry_options_set_database_path(options, TCHAR_TO_UTF8(*dbPath));
#endif

	// Location of the crashpad_backend.exe on windows
	// This must match the SentryClient.build.cs paths
#if PLATFORM_WINDOWS
	if (CrashPadLocation.IsEmpty())
	{
		auto Plugin = IPluginManager::Get().FindPlugin(SENTRY_PLUGIN_NAME);
		FString BaseDir = Plugin->GetBaseDir();
		CrashPadLocation = FPaths::Combine(BaseDir,
			TEXT("Binaries/ThirdParty/sentry-native/Win64/bin/crashpad_handler.exe")
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

	// We want sentry to send the log with any crash
	// TODO: make it possible to only add this with crashes, not other events?
	FString logfile = FGenericPlatformOutputDevices::GetAbsoluteLogFilename();
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

	// Set a before-send-callback, to flush logging
	sentry_options_set_before_send(options, &SentryEventFunction, (void*)this);

	// Todo: set up a thing to add log breadcrumbs

	UE_LOG(LogSentryClient, Log, TEXT("Initializing with DSN '%s'"), 
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

void FSentryClientModule::SetupContext()
{
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
	
	FString Name = FApp::GetName();
	sentry_value_set_by_key(value, "app_name", sentry_value_new_string(TCHAR_TO_UTF8(*Name)));
	const TCHAR *Version = FApp::GetBuildVersion();
	sentry_value_set_by_key(value, "app_version", sentry_value_new_string(TCHAR_TO_UTF8(Version)));

	const TCHAR *CommandLine = FCommandLine::Get();
	sentry_value_set_by_key(value, "commandline", sentry_value_new_string(TCHAR_TO_UTF8(CommandLine)));


	sentry_set_context("app", value);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSentryClientModule, SentryClient)
