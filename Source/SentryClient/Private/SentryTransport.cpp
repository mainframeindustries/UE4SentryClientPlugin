#include "SentryTransport.h"
#include "SentryClientModule.h"

#include "Http.h"
#include "HttpModule.h"
#include "Misc/ScopeLock.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"


#if SENTRY_HAVE_PLATFORM

sentry_transport_t* FSentryTransport::New() {
	sentry_transport_t* transport;

	auto Self = MakeShared<FSentryTransport, ESPMode::ThreadSafe>();
	transport = sentry_transport_new(&_send_func);
	if (!transport)
	{
		return nullptr;
	}
	// keep object alive by owning a reference to itself.
	// this is effectively the sentry_sdk's own reference
	Self->Self = Self; 
	
	sentry_transport_set_state(transport, (void*)&Self.Get());
	sentry_transport_set_startup_func(transport, _startup_func);
	sentry_transport_set_flush_func(transport, _flush_func);
	sentry_transport_set_shutdown_func(transport, _shutdown_func);
	sentry_transport_set_free_func(transport, _free_func);
	
	return transport;
}


void FSentryTransport::ParseDSN(const FString &dsn)
{
	// see https://develop.sentry.dev/sdk/envelopes/
	// first part is everythin up to and including ://

	FString scheme, addr;
	dsn.Split(TEXT("://"), &scheme, &addr);
	
	// split on the @ char if found
	sentry_key.Empty();
	addr.Split(TEXT("@"), &sentry_key, &addr);

	// split sentry key
	sentry_secret.Empty();
	sentry_key.Split(TEXT(":"), &sentry_key, &sentry_secret);

	// split project id, from the host part
	FString project_id;
	addr.Split(TEXT("/"), &addr, &project_id);

	// and generate the url, envelope api
	// this is different from the api docs, that use a /store/ api
	sentry_url = FString::Printf(TEXT("%s://%s/api/%s/envelope/"), *scheme, *addr, *project_id);

	// the auth headers
	auth_prefix = TEXT("Sentry sentry_version=7, sentry_client=") TEXT(SENTRY_SDK_USER_AGENT);
	if (!sentry_key.IsEmpty())
		auth_prefix += FString::Printf(TEXT(", sentry_key=%s"), *sentry_key);

	// sending envelopes doesn't require the secret key
	if (!sentry_secret.IsEmpty())
		auth_prefix += FString::Printf(TEXT(", sentry_secret=%s"), *sentry_secret);

}


void FSentryTransport::send_func(sentry_envelope_t* envelope)
{
	if (!Started)
	{
		return;
	}
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(sentry_url);
	HttpRequest->SetVerb(TEXT("POST"));

	// standard headers
	// probably don't need the user agent
	// HttpRequest->SetHeader(TEXT("User-Agent"), TEXT(SENTRY_SDK_USER_AGENT));
	
	// serialized envelope is in custom sentry format, not json
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-sentry-envelope"));

	// example at see https://develop.sentry.dev/sdk/store/ uses sentry_timestamp
	// in the auth header, but the native sdk doesn't use it.  native sdk doesn't
	// use the secret key either, for that matter.
	HttpRequest->SetHeader(TEXT("X-Sentry-Auth"), auth_prefix);

	// Override the user agent, putting the client in here
	HttpRequest->SetHeader(TEXT("UserAgent"), TEXT(SENTRY_PLUGIN_NAME) TEXT(" For UE4"));


	// set the content, content-length handled automatically
	size_t outsize;
	ANSICHAR* data = sentry_envelope_serialize(envelope, &outsize);
	TArray<uint8> content((uint8*)data, outsize);
	sentry_string_free(data);
	sentry_envelope_free(envelope);
	HttpRequest->SetContent(content);

	HttpRequest->OnProcessRequestComplete().BindThreadSafeSP(this, &FSentryTransport::OnComplete);
	// we can ba called on any thread, so guard this
	{
		FScopeLock Lock(&CriticalSection);
		Requests.Add(HttpRequest);
		if (!HttpRequest->ProcessRequest())
		{
			// problem.  remove it again
			Requests.Pop();
		}
	}
}

int FSentryTransport::startup_func(const sentry_options_t* options)
{
	FString dsn = ANSI_TO_TCHAR(sentry_options_get_dsn(options));
	ParseDSN(dsn);
	Started = true;
	return 0;
}

int FSentryTransport::flush_func(uint64_t timeout_ms)
{
	// flush the transport.
	// We must tick our requests ourselves, can't rely on game thread for that
	// get start time
	double Deadline = FPlatformTime::Seconds() + (double)timeout_ms * 1e-3;
	for (;;)
	{
		// get a copy of all the requests and tick them.
		TArray< TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> > copy;
		{
			FScopeLock lock(&CriticalSection);
			copy = Requests;
		}
		if (!copy.Num())
		{
			return 0;
		}
		for (auto& request : copy)
		{
			request->Tick(0.01);
		}
		// tikcing will have removed them, probably, so we wait a bit
		if (Requests.Num())
		{
			if (FPlatformTime::Seconds() > Deadline)
			{
				return 1;
			}
			FPlatformProcess::Sleep(0.01);
		}
	}

	return 0;
}


int FSentryTransport::shutdown_func(uint64_t timeout_ms)
{
	Started = false;
	return flush_func(timeout_ms);
}

void FSentryTransport::free_func()
{
	Self.Reset();
}


void FSentryTransport::OnComplete(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, bool bSuccess)
{
	// Note: always runs on game thread!!!!
	int32 Index = 0;
	FScopeLock Lock(&CriticalSection);
	for (auto &Stored : Requests)
	{
		if (Stored.Get() == Request.Get())
		{
			Requests.RemoveAt(Index);
			return;
		}
		++Index;
	}
}

#endif // SENTRY_HAVE_PLATFORM
