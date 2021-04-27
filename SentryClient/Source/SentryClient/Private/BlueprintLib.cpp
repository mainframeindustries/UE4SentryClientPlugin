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


bool USentryBlueprintLibrary::Initialize(const FString& DSN, const FString& Environment, const FString& Release, bool IsConsentRequired)

{
	auto* module = FSentryClientModule::Get();
	if (module)
	{
		return module->SentryInit(*DSN,
			Environment.IsEmpty() ? nullptr : *Environment,
			Release.IsEmpty() ? nullptr : *Release,
			IsConsentRequired
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
void USentryBlueprintLibrary::CaptureMessage(ESentryLevel level, const FString &logger, const FString &message)
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


// Consent handling

/**
 * Get current state of user consent
 */
ESentryConsent USentryBlueprintLibrary::GetUserConsent()
{
	sentry_user_consent_t consent = sentry_user_consent_get();
	// offset by one, because sentry uses -1 in its enums
	return (ESentryConsent)((int)consent + 1);
}

/**
 * Give user consent
 */
void USentryBlueprintLibrary::SetUserConsent(ESentryConsent Consent)
{
	switch (Consent)
	{
	case ESentryConsent::UNKNOWN:
		sentry_user_consent_reset();
		break;
	case ESentryConsent::GIVEN:
		sentry_user_consent_give();
		break;
	case ESentryConsent::REVOKED:
		sentry_user_consent_revoke();
		break;
	}
}


// User information

void USentryBlueprintLibrary::SetUser(const FString& id, const FString& username, const FString& email)
{
	sentry_value_t user = sentry_value_new_object();
	sentry_value_set_by_key(user, "ip_address", sentry_value_new_string("{{auto}}"));
	if (!id.IsEmpty())
	{
		sentry_value_set_by_key(user, "id", sentry_value_new_string(TCHAR_TO_UTF8(*id)));
	}
	if (!username.IsEmpty())
	{
		sentry_value_set_by_key(user, "username", sentry_value_new_string(TCHAR_TO_UTF8(*username)));
	}

	if (!email.IsEmpty())
	{
		sentry_value_set_by_key(user, "email", sentry_value_new_string(TCHAR_TO_UTF8(*email)));
	}
	sentry_set_user(user);
}

void USentryBlueprintLibrary::ClearUser()
{
	sentry_remove_user();
}


// Context information
void USentryBlueprintLibrary::SetContext(const FString& key, const TMap<FString, FString> &Values)
{
	sentry_value_t values = sentry_value_new_object();
	for (auto& item : Values)
	{
		sentry_value_set_by_key(values, TCHAR_TO_UTF8(*item.Key), sentry_value_new_string(TCHAR_TO_UTF8(*item.Value)));
	}
	sentry_set_context(TCHAR_TO_UTF8(*key), values);
}

/**
 * Clear information about the current user
 */
void USentryBlueprintLibrary::ClearContext(const FString& key)
{
	sentry_remove_context(TCHAR_TO_UTF8(*key));
}


// Tag handling

void USentryBlueprintLibrary::SetTag(const FString& key, const FString& Value)
{
	sentry_set_tag(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*Value));
}

void USentryBlueprintLibrary::RemoveTag(const FString& key)
{
	sentry_remove_tag(TCHAR_TO_UTF8(*key));
}

// Breadcrumbs.  Currently support only "Default"
void USentryBlueprintLibrary::AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, const FString& level, const FString& StringData)
{
	sentry_value_t crumb;
	switch (type)
	{
	case ESentryBreadcrumbType::Default:
		crumb = sentry_value_new_breadcrumb("default", TCHAR_TO_UTF8(*message));
		break;
	default:
		return;
	}
	if (!category.IsEmpty())
	{
		sentry_value_set_by_key(crumb, "category", sentry_value_new_string(TCHAR_TO_UTF8(*category)));
	}
	if (!level.IsEmpty())
	{
		sentry_value_set_by_key(crumb, "level", sentry_value_new_string(TCHAR_TO_UTF8(*level)));
	}
	sentry_value_set_by_key(crumb, "data", sentry_value_new_string(TCHAR_TO_UTF8(*StringData)));
	sentry_add_breadcrumb(crumb);
}