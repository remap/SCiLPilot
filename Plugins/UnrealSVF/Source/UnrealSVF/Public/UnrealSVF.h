// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"
#define _WIN32_WINTNT_WIN10_RS2 1

#ifndef SVF_USED3D11
#define SVF_USED3D11 1
#endif

#if PLATFORM_ANDROID
#include <jni.h>
#endif

DECLARE_STATS_GROUP(TEXT("UnrealSVF"), STATGROUP_UnrealSVF, STATCAT_Advanced);

class FUnrealSVFModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

#if PLATFORM_ANDROID
    void Java_com_microsoft_mixedreality_mrcs_svf_JavaPlugin_onCreated(JNIEnv *env, jclass clazz, jobject javaPlugin, jobject assetManager);
#endif

    void *dllHandle = nullptr;
};