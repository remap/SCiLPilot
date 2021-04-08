// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"

#pragma warning(push)
#pragma warning(disable: 4191) // warning C4191: 'type cast' : unsafe conversion
#pragma warning(disable: 4996) // error C4996: 'GetVersionEx' : was declared deprecated

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

#include "comdef.h"

namespace UnrealSFV_Private {

    FString GetHrStr(HRESULT ForHr) {
        _com_error err(ForHr);
        LPCTSTR errMsg = err.ErrorMessage();
        return FString(errMsg);
    }

}


#undef DeleteFile
#undef MoveFile
#undef LoadString
#undef GetMessage

#pragma warning(pop)

#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
