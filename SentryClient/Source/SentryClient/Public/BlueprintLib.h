#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SentryClientModule.h"

#include "BlueprintLib.generated.h"


// various enums

UENUM(BlueprintType)
enum class ESentryLevel : uint8 {
	DEBUG = 0,
	INFO = 1  UMETA(DisplayName = "INFO"),
	WARNING = 2  UMETA(DisplayName = "WARNING"),
	ERROR = 3  UMETA(DisplayName = "ERROR"),
	FATAL = 4  UMETA(DisplayName = "FATAL"),
};

UENUM(BlueprintType)
enum class ESentryConsent : uint8 {
	UNKNOWN = 0,
	GIVEN = 2,
	REVOKED = 1,
};

UENUM(BlueprintType)
enum class ESentryBreadcrumbType : uint8
{
	Default = 0,
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

	// Close the sentry client
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void Close();

	// Capture message
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
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void SetUser(const FString &id, const FString &username, const FString &email);

	/**
	 * Clear information about the current user
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void ClearUser();


	// Context information
	/**
	 * Add Context information
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void SetContext(const FString& key, const TMap<FString, FString> &Values);

	/**
	 * Clear information about the current user
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void ClearContext(const FString &key);

	// Tag information information
	/**
	 * Add Tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void SetTag(const FString& key, const FString& Value);

	/**
	 * Remove a tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void RemoveTag(const FString& key);

	/**
	 * Breadcrumbs
	 */
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
		const FString& _category, const FString& level, const FString& StringData);
		
};

