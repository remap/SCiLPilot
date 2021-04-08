// Copyright (C) Microsoft Corporation. All rights reserved.
//#define ANDROID_ARM64

using UnrealBuildTool;
using System;
using System.IO;

public class UnrealSVF : ModuleRules
{

    private string ModulePath
    {
        get
        {
            return ModuleDirectory;
        }
    }

    public UnrealSVF(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "RenderCore",
#if !UE_4_22_OR_LATER
                "ShaderCore",
#endif
                "RHI",
                "MediaAssets",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "ImageWrapper",
                "Json",
                "JsonUtilities",
                "Projects",
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
            }
        );

        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/inc")));
        string LibPath = Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/"));

        if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add("mf.lib");
            PublicAdditionalLibraries.Add("mfplat.lib");
            PublicAdditionalLibraries.Add("setupapi.lib");
            LibPath += Target.Platform == UnrealTargetPlatform.Win64 ? "x64/" : "x86/";
            PublicSystemIncludePaths.Add(LibPath);
            PublicAdditionalLibraries.Add("SVF.lib");
            PublicDelayLoadDLLs.Add("SVF.dll");
            RuntimeDependencies.Add(LibPath + "SVF.dll");

            // Set to true for use D3D11
            PublicDefinitions.Add("SVF_USED3D11=1");
            PublicDependencyModuleNames.Add("D3D11RHI");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicDependencyModuleNames.Add("Launch");
            PublicDependencyModuleNames.Add("RHI");
            PrivateDependencyModuleNames.Add("OpenGLDrv");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");

#if ANDROID_ARM64
            LibPath = Path.Combine(LibPath, "ARM64");
#else
            LibPath = Path.Combine(LibPath, "ARM");
#endif
            string enginePath = Path.GetFullPath(Path.GetFullPath(Target.RelativeEnginePath));
            PrivateIncludePaths.Add(Path.Combine(enginePath, "Source/Runtime/OpenGLDrv/Private"));

            AdditionalPropertiesForReceipt.Add(new ReceiptProperty(
                "AndroidPlugin", Path.Combine(ModulePath, "UnrealSVF_APL.xml")));
            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libSVFUnityPlugin.so"));
        }
        PublicLibraryPaths.Add(LibPath);
    }
}
