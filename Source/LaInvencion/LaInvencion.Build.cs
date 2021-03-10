// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LaInvencion : ModuleRules
{
	public LaInvencion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay"});
        
        PrivateDependencyModuleNames.AddRange(new string[] { "DDManager", "DDBase", "DDLog"});
	}
}
