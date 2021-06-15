#include "BlueprintLib.h"


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

void USentryBlueprintLibrary::SetVerbosity(ESentryVerbosity Verbosity)
{
	auto* module = FSentryClientModule::Get();
	if (module)
	{
		module->SetVerbosity((ELogVerbosity::Type)Verbosity);
	}
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
void USentryBlueprintLibrary::AddBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, const ESentryLevel level)
{
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_add_breadcrumb(crumb);
}

// Breadcrumbs.  Currently support only "Default"
void USentryBlueprintLibrary::AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level, const FString& StringData)
{
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_value_set_by_key(crumb, "data", sentry_value_new_string(TCHAR_TO_UTF8(*StringData)));
	sentry_add_breadcrumb(crumb);
}

// Breadcrumbs.  Currently support only "Default"
void USentryBlueprintLibrary::AddMapBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level, const TMap<FString, FString>& MapData)
{
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_value_t data = sentry_value_new_object();
	for(const auto &pair : MapData)
	{ 
		sentry_value_set_by_key(data, TCHAR_TO_UTF8(*pair.Key), sentry_value_new_string(TCHAR_TO_UTF8(*pair.Value)));
	}
	sentry_value_set_by_key(crumb, "data", data);
	sentry_add_breadcrumb(crumb);
}

sentry_value_t USentryBlueprintLibrary::BreadCrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level)
{
	sentry_value_t crumb;
	const ANSICHAR* ctype = "default";
	switch (type)
	{
	case ESentryBreadcrumbType::Default:
		ctype = "default";
		break;
	case ESentryBreadcrumbType::Debug:
		ctype = "debug";
		break;
	case ESentryBreadcrumbType::Error:
		ctype = "error";
		break;
	case ESentryBreadcrumbType::Navigation:
		ctype = "navigation";
		break;
	case ESentryBreadcrumbType::Http:
		ctype = "http";
		break;
	case ESentryBreadcrumbType::Info:
		ctype = "info";
		break;
	case ESentryBreadcrumbType::Query:
		ctype = "query";
		break;
	case ESentryBreadcrumbType::Transaction:
		ctype = "transaction";
		break;
	case ESentryBreadcrumbType::Ui:
		ctype = "ui";
		break;
	case ESentryBreadcrumbType::User:
		ctype = "user";
		break;
	}

	const ANSICHAR* clevel = nullptr;
	switch (level)
	{
	case ESentryLevel::SENTRY_FATAL:
		clevel = "fatal";
		break;
	case ESentryLevel::SENTRY_ERROR:
		clevel = "error";
		break;
	case ESentryLevel::SENTRY_WARNING:
		clevel = "warning";
		break;
	case ESentryLevel::SENTRY_INFO:
		clevel = "info";
		break;
	case ESentryLevel::SENTRY_DEBUG:
		clevel = "debug";
		break;
	}

	crumb = sentry_value_new_breadcrumb(ctype, TCHAR_TO_UTF8(*message));
	if (!category.IsEmpty())
	{
		sentry_value_set_by_key(crumb, "category", sentry_value_new_string(TCHAR_TO_UTF8(*category)));
	}
	if (clevel != nullptr)
	{
		sentry_value_set_by_key(crumb, "level", sentry_value_new_string(clevel));
	}
	return crumb;
}