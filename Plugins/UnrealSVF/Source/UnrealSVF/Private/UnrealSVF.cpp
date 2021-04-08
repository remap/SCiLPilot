// Copyright (C) Microsoft Corporation. All rights reserved.

#include "UnrealSVF.h"

#if PLATFORM_WINDOWS
#include "Misc/Paths.h"
#include "SVF.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Interfaces/IPluginManager.h"
#endif
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#endif

#include <memory>

#define LOCTEXT_NAMESPACE "FUnrealSVFModule"

#if !PLATFORM_ANDROID
#pragma comment(lib,"version")
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSVFModule, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFModule, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFModule, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFModule, Fatal, TEXT(pmt), ##__VA_ARGS__)

#if PLATFORM_WINDOWS
struct SVFVersion
{
    DWORD major, minor, patch, build;
};

extern "C" HRESULT SVFEXPORT GetSVFVersion(SVFVersion & outVersion);
#endif

#if PLATFORM_ANDROID
extern "C" void Java_com_microsoft_mixedreality_mrcs_svf_JavaPlugin_onCreatedUE4(JNIEnv* env, jclass clazz, jobject javaPlugin, jobject assetManager)
{
    env = FAndroidApplication::GetJavaEnv();
    // Not sure if we need to do this or not:
    //clazz = FAndroidApplication::FindJavaClass("com/SVFUnityPlugin/JavaPlugin");
    Java_com_microsoft_mixedreality_mrcs_svf_JavaPlugin_onCreated(env, clazz, javaPlugin, assetManager);
    //FAndroidApplication::CheckJavaException();
}
#endif


void FUnrealSVFModule::StartupModule()
{
#if PLATFORM_WINDOWS
#if PLATFORM_64BITS
    FString svfDir = IPluginManager::Get().FindPlugin("UnrealSVF")->GetBaseDir() + TEXT("/ThirdParty/x64/SVF.dll");
#elif PLATFORM_32BITS
    FString svfDir = IPluginManager::Get().FindPlugin("UnrealSVF")->GetBaseDir() + TEXT("/ThirdParty/x86/SVF.dll");
#endif

    if (FPaths::FileExists(svfDir))
    {
        dllHandle = FPlatformProcess::GetDllHandle(*svfDir);
        if (dllHandle == nullptr)
        {
            FatalSVF("Failed to get DllHandle for SVF");
        }
    }
    else
    {
        FatalSVF("SVF library not found at: %s", *svfDir);
    }

    SVFVersion version = {};
    if (SUCCEEDED(GetSVFVersion(version)))
    {
        if (version.major == SVF_RELEASE_VER_MAJOR &&
            version.minor == SVF_RELEASE_VER_MINOR &&
            version.patch == SVF_RELEASE_VER_PATCH &&
            version.build == SVF_RELEASE_VER_BUILD)
        {
            LogSVF("UnrealSVF Module loaded successfully with correct DLL version");
        }
        else
        {
            FatalSVF("UnrealSVF version (%d.%d.%d.%d) and SVF.dll version (%d.%d.%d.%d) mismatch!",
                SVF_RELEASE_VER_MAJOR, SVF_RELEASE_VER_MINOR, SVF_RELEASE_VER_PATCH, SVF_RELEASE_VER_BUILD,
                version.major, version.minor, version.patch, version.build);
        }
    }
    else
    {
        LogSVF("UnrealSVF failed to get SVF.dll version");
    }
#endif

#if PLATFORM_ANDROID && !WITH_EDITOR
    // Turn off RHI Thread because we have issues on OpenGL at the moment
    auto RHITCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.OpenGL.AllowRHIThread"));
    RHITCVar->Set(0, EConsoleVariableFlags::ECVF_SetByProjectSetting);

    // Copy out all files in Content/Movies
    const FString CopySrcRoot = FPaths::ProjectContentDir() / "Movies";
    IFileManager* FileManager = &IFileManager::Get();
    TArray<FString> IgnoreDirs; // Empty; Don't ignore any directories
    FLocalTimestampDirectoryVisitor Visitor(FPlatformFileManager::Get().GetPlatformFile(),
        IgnoreDirs, IgnoreDirs, false);
    FileManager->IterateDirectory(*CopySrcRoot, Visitor);

    // For some reason this iterator goes over the same file twice in the form of
    // 1) ../../../[ProjectName]/Content/Movies
    // 2) [ProjectName]/Content/Movies/
    // on certain platforms.
    FString DirPrefix = FPaths::ProjectContentDir();
    while (DirPrefix.RemoveFromStart(TEXT("../"), ESearchCase::IgnoreCase)) {}

    LogSVF("Copying all *.mp4 files in Content/Movies to external file path");
    for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
    {
        FString SourceFilename = TimestampIt.Key();
        while (SourceFilename.RemoveFromStart(TEXT("../"), ESearchCase::IgnoreCase)) {}
        if (FPaths::GetExtension(SourceFilename, false) != "mp4")
        {
            continue;
        }
        if (!SourceFilename.RemoveFromStart(DirPrefix, ESearchCase::IgnoreCase))
        {
            WarnSVF("File %s was not copied since it didn't have expected file path", *SourceFilename);
            continue;
        }
        FString DestFilename = GExternalFilePath / SourceFilename;
        TArray<uint8> CopiedData;
        if (!FPaths::FileExists(DestFilename))
        {
            if (FFileHelper::LoadFileToArray(CopiedData, *TimestampIt.Key(), 0))
            {
                FFileHelper::SaveArrayToFile(CopiedData, *DestFilename);
                LogSVF("%s copied to external path %s", *SourceFilename, *DestFilename);
            }
            else
            {
                WarnSVF("Failed to copy %s", *SourceFilename);
            }
        }
        else
        {
            LogSVF("Did not copy %s as it already exists", *SourceFilename);
        }
    }
#endif
}

void FUnrealSVFModule::ShutdownModule()
{
#if PLATFORM_WINDOWS
    FPlatformProcess::FreeDllHandle(dllHandle);
#endif
}

#undef LogSVF
#undef WarnSVF
#undef FatalSVF

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealSVFModule, UnrealSVF)
