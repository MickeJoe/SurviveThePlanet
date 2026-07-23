// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SurviveThePlanet : ModuleRules
{
	public SurviveThePlanet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"SurviveThePlanet",
			"SurviveThePlanet/Gameplay/Base",
			"SurviveThePlanet/Gameplay/Drones",
			"SurviveThePlanet/Gameplay/Planet",
			"SurviveThePlanet/Gameplay/Work",
			"SurviveThePlanet/Variant_Strategy",
			"SurviveThePlanet/Variant_Strategy/UI",
			"SurviveThePlanet/Variant_TwinStick",
			"SurviveThePlanet/Variant_TwinStick/AI",
			"SurviveThePlanet/Variant_TwinStick/Gameplay",
			"SurviveThePlanet/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
