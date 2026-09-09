#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "STPCheatManager.generated.h"

UCLASS(NotBlueprintable)
class SURVIVETHEPLANET_API USTPCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Adds Amount of ResourceType to the active resource manager. Development builds only. */
	UFUNCTION(BlueprintCallable, Exec, Category = "Cheats|Resources", meta = (DevelopmentOnly))
	bool GiveResource(EResourceType ResourceType, int32 Amount);

	/** Completes an active objective through the normal completion and reward flow. */
	UFUNCTION(BlueprintCallable, Exec, Category = "Cheats|Objectives", meta = (DevelopmentOnly))
	bool CompleteObjective(FName ObjectiveId = TEXT("mission_build_communication"));

	/** Grants one construction blueprint to the player inventory. */
	UFUNCTION(BlueprintCallable, Exec, Category = "Cheats|Blueprints", meta = (DevelopmentOnly))
	bool GrantBuildingBlueprint(ESTPBuildTool BuildTool);
};
