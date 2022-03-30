#pragma once

// include the sentry client if we have it

#if PLATFORM_WINDOWS
#define SENTRY_HAVE_PLATFORM 1
#elif PLATFORM_LINUX
#define SENTRY_HAVE_PLATFORM 1
#else
#define SENTRY_HAVE_PLATFORM 0
#endif

#if SENTRY_HAVE_PLATFORM

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#include "sentry.h"

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#endif