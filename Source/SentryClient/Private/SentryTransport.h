#pragma once

#include "SentryCore.h"

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "HAL/CriticalSection.h"


class FSentryTransport : public TSharedFromThis<FSentryTransport, ESPMode::ThreadSafe>
{
public:
	
	// create a new transport
	static sentry_transport_t* New();
	~FSentryTransport()
	{
		;
	}

private:
	// the transport api hook functions, thunkers and members
	static void _send_func(sentry_envelope_t* envelope, void* state)
	{
		return static_cast<FSentryTransport*>(state)->send_func(envelope);
	}
	static int _startup_func(const sentry_options_t* options, void* state)
	{
		return static_cast<FSentryTransport*>(state)->startup_func(options);
	}
	static int _shutdown_func(uint64_t timeout, void* state)
	{
		return static_cast<FSentryTransport*>(state)->shutdown_func(timeout);
	}
	static void _free_func(void* state)
	{
		return static_cast<FSentryTransport*>(state)->free_func();
	}

	void send_func(sentry_envelope_t* envelope);
	int startup_func(const sentry_options_t* options);
	int shutdown_func(uint64_t timeout);
	void free_func();

private:
	
	void ParseDSN(const FString& dsn);

	/**
	 * Callback from the HttpRequest.
	 * Called when an Http request completes.
	 */
	void OnComplete(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request, TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response, bool bSuccess);

	// todo: Support some sort of rate limiting
	float maxRate = 0.0;

	// these are computed from the dsn
	FString sentry_url;
	FString sentry_key;
	FString sentry_secret;
	FString auth_prefix;

	// currently executing requests
	TArray<TSharedPtr < IHttpRequest, ESPMode::ThreadSafe > > Requests;
	FCriticalSection CriticalSection;

	// we own a reference to ourself.  This allows the use of shared
	// pointers while still controlling lifetime from the sdk lib
	TSharedPtr<FSentryTransport, ESPMode::ThreadSafe> Self;
};
