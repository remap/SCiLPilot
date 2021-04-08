// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#endif

#pragma warning(push)
#pragma warning(disable: 4191)		// warning C4191: 'type cast' : unsafe conversion
#pragma warning(disable: 4996)		// error C4996: 'GetVersionEx' : was declared deprecated

#ifndef DeleteFile
#define DeleteFile DeleteFileW
#endif

#ifndef MoveFile
#define MoveFile MoveFileW
#endif

#ifndef LoadString
#define LoadString LoadStringW
#endif

#ifndef GetMessage
#define GetMessage GetMessageW
#endif

