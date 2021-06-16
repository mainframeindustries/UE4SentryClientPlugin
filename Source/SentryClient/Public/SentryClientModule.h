#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "sentry.h"
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "SentryClientModule.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSentryClient, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSentryCore, Log, All);

// use this to figure out the plugin folder at runtime.
// if the name of this plugin changes, it needs to be reflected here.
#define SENTRY_PLUGIN_NAME "SentryClient"

class FSentryClientModule;

// Class for binding to the GLog and funneling messages .
class FSentryOutputDevice : public FOutputDevice
{
public:
	FSentryOutputDevice(FSentryClientModule* _module) : module(_module) {}

	// Sentry handler is thread safe
	virtual bool CanBeUsedOnAnyThread() const
	{
		return true;
	}
	virtual bool CanBeUsedOnMultipleThreads() const
	{
		return true;
	}

	// Serialize, which runs on tick, and notifies if a new message has been created.
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override;
private:
	FSentryClientModule* module;
};

class FSentryClientModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


	bool SentryInit(const TCHAR* DNS, const TCHAR* Environment, const TCHAR* Release, bool IsConsentRequired);
	void SentryClose();

	void SetupContext();

	// get the module if it exists
	static FSentryClientModule* Get();
	bool IsInitialized() const { return initialized; }

	static void SentryLog(int level, const char* message, va_list args);

	ELogVerbosity::Type GetVerbosity() const { return Verbosity; }
	void SetVerbosity(ELogVerbosity::Type _Verbosity) { Verbosity = _Verbosity; }

private:
	bool initialized = false;
	FString dbPath;
	FString CrashPadLocation;
	TSharedPtr<FSentryOutputDevice> LogDevice;
	ELogVerbosity::Type Verbosity = ELogVerbosity::Warning;  // only report warnings or worse as breadcrumbs
};



// A config class for the game.
// Defaults can be saved in DefaultSentry.ini (or other Sentry.ini files)
// under the key following key and example:
// [/Script/SentryClient.SentryClientConfig]
// DSN=https://YOUR_KEY@oORG_ID.ingest.sentry.io/PROJECT_ID
// Release=v.0.1.9
//
// See https://docs.unrealengine.com/en-US/ProductionPipelines/ConfigurationFiles/index.html for information on the config file hierarchy

UCLASS(Config = Sentry)
class USentryClientConfig : public UObject
{
	GENERATED_BODY()
public:

	// Crashpad is enabled by default, if there is a DSN.  Can be disabled via .ini files
	UPROPERTY(Config)
	bool Enabled=true;

	// Default DSN unless overridden with env or command line
	UPROPERTY(Config)
	FString DSN;

	UPROPERTY(Config)
	FString Environment;

	UPROPERTY(Config);
	FString Release;

	UPROPERTY(Config);
	bool ConsentRequired = false;


	static FString GetEnvOrCmdLine(const TCHAR* name);

	static bool IsEnabled();
	static FString GetDSN();
	static FString GetEnvironment();
	static FString GetRelease();
	static bool IsConsentRequired();

	static USentryClientConfig* Get() { return CastChecked<USentryClientConfig>(USentryClientConfig::StaticClass()->GetDefaultObject()); }
};