#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "SentryClientModule.h"

#include "BlueprintLib.generated.h"


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

};

