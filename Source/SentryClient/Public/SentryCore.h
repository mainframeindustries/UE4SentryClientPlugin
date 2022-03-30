#pragma once

// include the sentry client if we have it


#define PLATFORM_HAS_SENTRY 1

#if PLATFORM_HAS_SENTRY

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#include "sentry.h"

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#endif