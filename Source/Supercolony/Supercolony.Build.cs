// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Supercolony : ModuleRules
{
	public Supercolony(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets files include across system folders, e.g. "Pheromones/PheromoneGrid.h" from Tests/.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DeveloperSettings", "MassEntity", "MassCore", "MassCommon", "MassSpawner", "MassMovement", "MassRepresentation" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
