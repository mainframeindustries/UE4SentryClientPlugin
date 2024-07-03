#pragma once

#include "SentryCore.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Misc/OutputDeviceError.h"

#include "SentryClientModule.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSentryClient, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogSentryCore, Verbose, All);

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

	// actual serialization function
	static void StaticSerialize(const TCHAR* Msg, ELogVerbosity::Type Verbosity, const class FName& Category);

private:
	FSentryClientModule* module;
};

class FSentryErrorOutputDevice : public FOutputDeviceError
{
public:
	/**
	 * Serializes the passed in data unless the current event is suppressed.
	 *
	 * @param	Data	Text to log
	 * @param	Event	Event name used for suppression purposes
	 */
	virtual void Serialize(const TCHAR* Msg, ELogVerbosity::Type Verbosity, const class FName& Category) override;

	/**
	 * Error handling function that is being called from within the system wide global
	 * error handler, e.g. using structured exception handling on the PC.
	 */
	virtual void HandleError() override;

	void SetParent(FOutputDeviceError* _p) { parent = _p; }
private:
	FOutputDeviceError* parent = nullptr;
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
	static sentry_value_t SentryCrash(const sentry_ucontext_t* uctx, sentry_value_t event);

	ELogVerbosity::Type GetVerbosity() const { return Verbosity; }
	void SetVerbosity(ELogVerbosity::Type _Verbosity) { Verbosity = _Verbosity; }

private:
	bool initialized = false;
	FString dbPath;
	FString CrashPadLocation;
	TSharedPtr<FSentryOutputDevice> LogDevice;
	ELogVerbosity::Type Verbosity = ELogVerbosity::Warning;  // only report warnings or worse as breadcrumbs
	static FSentryErrorOutputDevice ErrorDevice;
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

	UPROPERTY(Config);
	bool SubmitUserFeedback = true;

	UPROPERTY(Config);
	FString Tags;	// comma separated list of key=value tags

	// return an environment or command line option.  Return true if found.
	static bool GetEnvOrCmdLine(const TCHAR* name, FString &out);
	static FString GetEnvOrCmdLine(const TCHAR* name);
	static FString GetConfig(const TCHAR* name, const TCHAR *defaultval);

	static bool IsEnabled();
	static bool ShouldDisable();
	static FString GetDSN();
	static FString GetEnvironment();
	static FString GetRelease();
	static bool IsConsentRequired();
	// get tags passed as env or command line
	static TMap<FString, FString> GetTags();
	// get Database path
	static FString GetDatabasePath();

	static USentryClientConfig* Get() { return CastChecked<USentryClientConfig>(USentryClientConfig::StaticClass()->GetDefaultObject()); }
};