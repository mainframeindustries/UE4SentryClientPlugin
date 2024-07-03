#pragma once
#include "SentryClientModule.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SentryCore.h"

#include "BlueprintLib.generated.h"


#if !SENTRY_HAVE_PLATFORM
// need a dummy sentry value
class sentry_value_t
{};
#endif

// various enums

UENUM(BlueprintType)
enum class ESentryLevel : uint8 {
	SENTRY_DEBUG = 0 UMETA(DisplayName = "Debug"),
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


/**
 * A class that represents a sentry generic value
 */
UCLASS(BlueprintType)
class SENTRYCLIENT_API USentryValue : public UObject
{
	GENERATED_BODY()
public:
	USentryValue();
	USentryValue(sentry_value_t);
	~USentryValue();


	/**
	 * Generate a new null sentry value
	 */
	UFUNCTION(BlueprintPure, Category = "Sentry|Value")
	static USentryValue *CreateValueNull();
	
	/**
	 * Generate a new string sentry value
	 */
	UFUNCTION(BlueprintPure, Category = "Sentry|Value")
	static USentryValue* CreateValueString(const FString &str);

	/**
	 * Generate a new int sentry value
	 */
	UFUNCTION(BlueprintPure, Category = "Sentry|Value")
	static USentryValue* CreateValueInt(int32 i);

	/**
	 * Generate a new float sentry value
	 */
	UFUNCTION(BlueprintPure, Category = "Sentry|Value")
	static USentryValue* CreateValueFloat(float f);

	/**
	 * Generate a new bool sentry value
	 */
	UFUNCTION(BlueprintPure, Category = "Sentry|Value")
	static USentryValue* CreateValueBool(bool b);

	/**
	* Generate a new object sentry value from a map of values
	*/
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	static USentryValue* CreateValueObject(const TMap<FString, USentryValue*>& map);

	/**
	* Generate a new list sentry value from an array of values
	*/
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	static USentryValue* CreateValueList(const TArray<USentryValue*>& array);

	/**
	 * Generate a new empty object sentry value
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	static USentryValue* CreateEmptyValueObject();

	/**
	 * Generate a new empty list sentry value
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	static USentryValue* CreateEmptyValueList();

	/**
	 * Append an value to a sentry list
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	void Append(USentryValue *v);

	/**
	 * Set a key on an object sentry value
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Value")
	void Set(const FString &key, USentryValue* value);

#if SENTRY_HAVE_PLATFORM
	/* transfer refernce out of object.  set null object inside*/
	sentry_value_t Transfer();
	/* get new reference for object inside */
	sentry_value_t Get() const;
	/* Take ownership of incoming reference */
	void Take(sentry_value_t v);
	

private:
	sentry_value_t value;
#endif
};


UCLASS()
class SENTRYCLIENT_API USentryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Is the plugin initialized
	UFUNCTION(BlueprintPure, Category = "Sentry")
	static bool IsInitialized();

	// Is the plugin implemented for the current platform?
	UFUNCTION(BlueprintPure, Category = "Sentry")
	static bool IsImplemented();

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
	 * @param value A string:string map representing the context
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Context")
	static void SetContext(const FString& key, const TMap<FString, FString> &Values);

	/**
	 * Add Context information
	 * See https://docs.sentry.io/platforms/native/enriching-events/context/
	 * @param value A Sentry Value Object representing the context.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Context")
	static void SetContextObject(const FString& key, USentryValue *value);

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
	const FString& _category, ESentryLevel level);
	/**
	 * Add a breacrumb with a String as "data"
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
		const FString& _category, ESentryLevel level, const FString& StringData);
	/**
	 * Add a breacrumb with a String as "data"
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddMapBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& _category, ESentryLevel level, const TMap<FString, FString>& MapData);
		
	/**
	 * Add a breacrumb with a Sentry Value as data
	 * See https://docs.sentry.io/platforms/native/enriching-events/breadcrumbs/
	 * and https://develop.sentry.dev/sdk/event-payloads/breadcrumbs/#breadcrumb-types
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|Breadcrumb")
	static void AddValueBreadcrumb(ESentryBreadcrumbType type, const FString& message,
			const FString& _category, ESentryLevel level, USentryValue *value);

	/**
	 * Creates a new User Feedback with a specific name, email and comments.
	 * See https://develop.sentry.dev/sdk/envelopes/#user-feedback
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry|UserFeedback")
	static void SubmitUserFeedback(const FString& Name, const FString& EventMessage, const FString& Comments);

};
