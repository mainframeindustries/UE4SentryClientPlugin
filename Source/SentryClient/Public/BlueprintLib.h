#pragma once
#include "SentryClientModule.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "sentry.h"
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "BlueprintLib.generated.h"


// various enums

UENUM(BlueprintType)
enum class ESentryLevel : uint8 {
	SENTRY_DEBUG = 0,
	SENTRY_INFO = 1  UMETA(DisplayName = "Info"),
	SENTRY_WARNING = 2  UMETA(DisplayName = "Warning"),
	SENTRY_ERROR = 3  UMETA(DisplayName = "Error"),
	SENTRY_FATAL = 4  UMETA(DisplayName = "Fatal"),
};

UENUM(BlueprintType)
enum class ESentryConsent : uint8 {
	UNKNOWN = 0 UMETA(DisplayName = "Unknown"),
	GIVEN = 2 UMETA(DisplayName = "Given"),
	REVOKED = 1 UMETA(DisplayName = "Revoked"),
};

UENUM(BlueprintType)
enum class ESentryBreadcrumbType : uint8
{
	Default = 0,
	Debug,
	Error,
	Navigation,
	Http,
	Info,
	Query,
	Transaction,
	Ui,
	User,
};

// re-create the verbosity as enum

UENUM(BlueprintType)
enum class ESentryVerbosity : uint8
{
	_NoLogging = 0 UMETA(DisplayName = "NoLogging"),
	_Fatal = ELogVerbosity::Fatal UMETA(DisplayName = "Fatal"),
	_Error = ELogVerbosity::Error UMETA(DisplayName = "Error"),
	_Warning = ELogVerbosity::Warning UMETA(DisplayName = "Warning"),
	_Display = ELogVerbosity::Display UMETA(DisplayName = "Display"),
	_Log = ELogVerbosity::Log UMETA(DisplayName = "Log"),
	_Verbose = ELogVerbosity::Verbose UMETA(DisplayName = "Verbose"),
	_VeryVerbose = ELogVerbosity::VeryVerbose UMETA(DisplayName = "VeryVerbose"),
};

UCLASS()
class USentryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	
	UFUNCTION(BlueprintPure, Category = "Sentry")
	static bool IsInitialized();

	// Initialize the sentry client
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static bool Initialize(const FString &DSN, const FString &Environment, const FString &Release, bool IsConsentRequired);

	/** 
	 * Set verbosity for breadcrumbs
	 * @param Verbosity The highest verbosity logs that will be sent as breadcrumbs
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	void SetVerbosity(ESentryVerbosity Verbosity);

	// Close the sentry client
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void Close();

	/** Capture message
	 * See https://docs.sentry.io/platforms/native/usage/
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Event")
	static void CaptureMessage(ESentryLevel level, const FString &logger, const FString &message);

	/**
	 * Get current state of user consent
	 */
	UFUNCTION(BluePrintPure, Category = "Sentry|Consent")
	static ESentryConsent GetUserConsent();

	/**
	 * Give user consent
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Consent")
	static void SetUserConsent(ESentryConsent Consent);

	
	// User information
	/**
	 * Add information about the current user
	 * see https://docs.sentry.io/platforms/native/enriching-events/identify-user/
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|User")
	static void SetUser(const FString &id, const FString &username, const FString &email);

	/**
	 * Clear information about the current user
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|User")
	static void ClearUser();


	// Context information
	/**
	 * Add Context information
	 * See https://docs.sentry.io/platforms/native/enriching-events/context/
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Context")
	static void SetContext(const FString& key, const TMap<FString, FString> &Values);

	/**
	 * Clear information about the current user
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Context")
	static void ClearContext(const FString &key);

	// Tag information information
	/**
	 * Add Tag
	 * See https://docs.sentry.io/platforms/native/enriching-events/tags/
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Tag")
	static void SetTag(const FString& key, const FString& Value);

	/**
	 * Remove a tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Tag")
	static void RemoveTag(const FString& key);

	/**
	 * Add a breacrumb
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& _category, const FString& level);
	/**
	 * Add a breacrumb with a String as "data"
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
		const FString& _category, const FString& level, const FString& StringData);
	/**
	 * Add a breacrumb with a String as "data"
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddMapBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& _category, const FString& level, const TMap<FString, FString>& MapData);
		

	// helper function
	static sentry_value_t BreadCrumb(ESentryBreadcrumbType type, const FString& message,
		const FString& _category, const FString& level);
};

