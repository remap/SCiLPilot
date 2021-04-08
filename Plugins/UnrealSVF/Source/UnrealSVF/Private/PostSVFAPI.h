// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#undef DeleteFile
#undef MoveFile
#undef LoadString
#undef GetMessage

#pragma warning(pop)

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

