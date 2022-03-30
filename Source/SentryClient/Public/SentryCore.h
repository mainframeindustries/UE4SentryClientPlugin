#pragma once

// include the sentry client if we have it


#define SENTRY_HAVE_PLATFORM 1

#if SENTRY_HAVE_PLATFORM

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#include "sentry.h"

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#endif