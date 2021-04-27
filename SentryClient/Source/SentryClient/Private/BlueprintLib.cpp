// Copyright Epic Games, Inc. All Rights Reserved.

#include "BlueprintLib.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#define SENTRY_BUILD_STATIC 1 
#include "sentry.h"
#include "Windows/HideWindowsPlatformTypes.h"


bool USentryBlueprintLibrary::IsInitialized()
{
	auto* module = FSentryClientModule::Get();
	if (module)
	{
		return module->IsInitialized();
	}
	return false;
}


bool USentryBlueprintLibrary::Initialize(const FString& DSN, const FString& Environment, const FString& Release)

{
	auto* module = FSentryClientModule::Get();
	if (module)
	{
		return module->SentryInit(*DSN,
			Environment.IsEmpty() ? nullptr : *Environment,
			Release.IsEmpty() ? nullptr : *Release
		);
	}
	return false;
}


void USentryBlueprintLibrary::Close()

{
	auto* module = FSentryClientModule::Get();
	if (module)
	{
		return module->SentryClose();
	}
}


// Capture message
void USentryBlueprintLibrary::CaptureMessage(SentryLevel level, const FString &logger, const FString &message)
{
	// sentry uses negative enums as well, weird.
	sentry_level_t l = (sentry_level_t)((int)level - 1);
	sentry_capture_event(
		sentry_value_new_message_event(
			l,
			TCHAR_TO_UTF8(*logger),
			TCHAR_TO_UTF8(*message)
		)
	);
}