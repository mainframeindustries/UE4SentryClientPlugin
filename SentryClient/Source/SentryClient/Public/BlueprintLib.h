#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SentryClientModule.h"

#include "BlueprintLib.generated.h"


// various enums

UENUM(BlueprintType)
enum class SentryLevel : uint8 {
	DEBUG = 0,
	INFO = 1  UMETA(DisplayName = "INFO"),
	WARNING = 2  UMETA(DisplayName = "WARNING"),
	ERROR = 3  UMETA(DisplayName = "ERROR"),
	FATAL = 4  UMETA(DisplayName = "FATAL"),
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
	static bool Initialize(const FString &DSN, const FString &Environment, const FString &Release);

	// Close the sentry client
	UFUNCTION(BlueprintCallable, Category = "Sentry")
	static void Close();

	// Capture message
	UFUNCTION(BlueprintCallable, Category = "Sentry|Event")
	static void CaptureMessage(SentryLevel level, const FString &logger, const FString &message);

};

