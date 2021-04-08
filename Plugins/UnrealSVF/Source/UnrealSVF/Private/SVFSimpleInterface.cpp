// Fill out your copyright notice in the Description page of Project Settings.

#include "SVFSimpleInterface.h"
#include "UnrealSVF.h"
#include "SVFClockInterface.h"

#if PLATFORM_WINDOWS
#include "SVFReaderPassThrough.h"
#elif PLATFORM_ANDROID
#include "SVFReaderAndroid.h"
#endif

// Add default functionality here for any ISVFSimpleInterface functions that are not pure virtual.

bool ISVFSimpleInterface::CreateInstance(UObject* InOwner, const FString& FilePath,
    const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject) {
#if PLATFORM_WINDOWS
    return USVFReaderPassThrough::CreateInstance(InOwner, FPaths::ProjectContentDir() / FilePath,
        OpenInfo, OutWrapper, CustomClockObject);
#elif PLATFORM_ANDROID
    return USVFReaderAndroid::CreateInstance(InOwner, FilePath, OpenInfo, OutWrapper, CustomClockObject);
#else
    return false;
#endif
}
