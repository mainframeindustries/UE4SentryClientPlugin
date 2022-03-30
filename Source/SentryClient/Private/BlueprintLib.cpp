#include "BlueprintLib.h"

// helper function
#if SENTRY_HAVE_PLATFORM
static sentry_value_t BreadCrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& _category, ESentryLevel level);
#endif

USentryValue::USentryValue()
{
#if SENTRY_HAVE_PLATFORM
	value = sentry_value_new_null();
#endif
}

// create a value taking ownership of value
USentryValue::USentryValue(sentry_value_t v)
{
#if SENTRY_HAVE_PLATFORM
	value = v;
#endif
}

USentryValue* USentryValue::CreateValueNull()
{
	return NewObject<USentryValue>();
}

USentryValue* USentryValue::CreateValueString(const FString& str)
{
	USentryValue* value = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	value->Take(sentry_value_new_string(TCHAR_TO_UTF8(*str)));
#endif
	return value;
}

USentryValue* USentryValue::CreateValueInt(int32 i)
{
	USentryValue* value = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	value->Take(sentry_value_new_int32(i));
#endif
	return value;
}

USentryValue* USentryValue::CreateValueFloat(float f)
{
	USentryValue* value = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	value->Take(sentry_value_new_double((double)f));
#endif
	return value;
	
}

USentryValue* USentryValue::CreateValueBool(bool b)
{
	USentryValue* value = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	value->Take(sentry_value_new_bool((int)b));
#endif
	return value;
}


USentryValue* USentryValue::CreateValueList(const TArray<USentryValue*>& l)
{
	USentryValue* result = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	auto value = sentry_value_new_list();
	for (auto *v : l)
	{
		sentry_value_append(value, v->Get());
	}
	result->Take(value);
#endif
	return result;
}
USentryValue* USentryValue::CreateEmptyValueList()
{
	USentryValue* result = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	auto value = sentry_value_new_list();
	result->Take(value);
#endif
	return result;
}


USentryValue* USentryValue::CreateValueObject(const TMap<FString, USentryValue*>& s)
{
	USentryValue* result = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	auto value = sentry_value_new_list();
	for (auto &e : s)
	{
		sentry_value_set_by_key(value, TCHAR_TO_UTF8(*e.Key), e.Value->Get());
	}
	result->Take(value);
#endif
	return result;
}

USentryValue* USentryValue::CreateEmptyValueObject()
{
	USentryValue* result = NewObject<USentryValue>();
#if SENTRY_HAVE_PLATFORM
	auto value = sentry_value_new_object();
	result->Take(value);
#endif
	return result;
}

void USentryValue::Append(USentryValue *v)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_append(value, v->Get());
#endif
}

void USentryValue::Set(const FString &key, USentryValue* v)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_set_by_key(value, TCHAR_TO_UTF8(*key), v->Get());
#endif
}


USentryValue::~USentryValue()
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_decref(value);
#endif
}

#if SENTRY_HAVE_PLATFORM

// transfer ownership of value
sentry_value_t USentryValue::Transfer()
{
	sentry_value_t res = value;
	value = sentry_value_new_null();
	return res;	
}

sentry_value_t USentryValue::Get() const
{
	sentry_value_incref(value);
	return value;
}

void USentryValue::Take(sentry_value_t v)
{
	sentry_value_decref(value);
	value = v;
}
#endif


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
#if SENTRY_HAVE_PLATFORM

	// sentry uses negative enums as well, weird.
	sentry_level_t l = (sentry_level_t)((int)level - 1);
	sentry_capture_event(
		sentry_value_new_message_event(
			l,
			TCHAR_TO_UTF8(*logger),
			TCHAR_TO_UTF8(*message)
		)
	);
#endif
}


// Consent handling

/**
 * Get current state of user consent
 */
ESentryConsent USentryBlueprintLibrary::GetUserConsent()
{
#if SENTRY_HAVE_PLATFORM
	sentry_user_consent_t consent = sentry_user_consent_get();
	// offset by one, because sentry uses -1 in its enums
	return (ESentryConsent)((int)consent + 1);
#else
	// no sentry, act as if consent is given
	return ESentryConsent::GIVEN;
#endif
}

/**
 * Give user consent
 */
void USentryBlueprintLibrary::SetUserConsent(ESentryConsent Consent)
{
#if SENTRY_HAVE_PLATFORM
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
#endif
}


// User information

void USentryBlueprintLibrary::SetUser(const FString& id, const FString& username, const FString& email)
{
#if SENTRY_HAVE_PLATFORM
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
#endif
}

void USentryBlueprintLibrary::ClearUser()
{
#if SENTRY_HAVE_PLATFORM
	sentry_remove_user();
#endif
}


// Context information
void USentryBlueprintLibrary::SetContext(const FString& key, const TMap<FString, FString> &Values)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_t values = sentry_value_new_object();
	for (auto& item : Values)
	{
		sentry_value_set_by_key(values, TCHAR_TO_UTF8(*item.Key), sentry_value_new_string(TCHAR_TO_UTF8(*item.Value)));
	}
	sentry_set_context(TCHAR_TO_UTF8(*key), values);
#endif
}

void USentryBlueprintLibrary::SetContextObject(const FString& key, USentryValue *value)
{
#if SENTRY_HAVE_PLATFORM
	sentry_set_context(TCHAR_TO_UTF8(*key), value->Get());
#endif
}

/**
 * Clear information about the current user
 */
void USentryBlueprintLibrary::ClearContext(const FString& key)
{
#if SENTRY_HAVE_PLATFORM
	sentry_remove_context(TCHAR_TO_UTF8(*key));
#endif
}


// Tag handling

void USentryBlueprintLibrary::SetTag(const FString& key, const FString& Value)
{
#if SENTRY_HAVE_PLATFORM
	sentry_set_tag(TCHAR_TO_UTF8(*key), TCHAR_TO_UTF8(*Value));
#endif
}

void USentryBlueprintLibrary::RemoveTag(const FString& key)
{
#if SENTRY_HAVE_PLATFORM
	sentry_remove_tag(TCHAR_TO_UTF8(*key));
#endif
}

// Breadcrumbs.  Currently support only "Default"
void USentryBlueprintLibrary::AddBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, const ESentryLevel level)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_add_breadcrumb(crumb);
#endif
}

// Breadcrumbs.  Currently support only "Default"
void USentryBlueprintLibrary::AddStringBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level, const FString& StringData)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_value_set_by_key(crumb, "data", sentry_value_new_string(TCHAR_TO_UTF8(*StringData)));
	sentry_add_breadcrumb(crumb);
#endif
}

// Breadcrumbs.  Currently support only "Default"

void USentryBlueprintLibrary::AddMapBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level, const TMap<FString, FString>& MapData)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_value_t data = sentry_value_new_object();
	for(const auto &pair : MapData)
	{ 
		sentry_value_set_by_key(data, TCHAR_TO_UTF8(*pair.Key), sentry_value_new_string(TCHAR_TO_UTF8(*pair.Value)));
	}
	sentry_value_set_by_key(crumb, "data", data);
	sentry_add_breadcrumb(crumb);
#endif
}

void USentryBlueprintLibrary::AddValueBreadcrumb(ESentryBreadcrumbType type, const FString& message,
	const FString& category, ESentryLevel level, USentryValue *value)
{
#if SENTRY_HAVE_PLATFORM
	sentry_value_t crumb = BreadCrumb(type, message, category, level);
	sentry_value_set_by_key(crumb, "data", value->Get());
	sentry_add_breadcrumb(crumb);
#endif
}

#if SENTRY_HAVE_PLATFORM
sentry_value_t BreadCrumb(ESentryBreadcrumbType type, const FString& message,
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

#endif // SENTRY_HAVE_PLATFORM
