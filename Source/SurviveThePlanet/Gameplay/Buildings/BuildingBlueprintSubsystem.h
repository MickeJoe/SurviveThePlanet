#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "BuildingBlueprintSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildingBlueprintInventoryChanged, ESTPBuildTool, BuildTool);

/** Runtime/save-ready inventory of construction blueprints owned by the player. */
UCLASS()
class SURVIVETHEPLANET_API UBuildingBlueprintSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Building Blueprints")
	bool OwnsBlueprint(ESTPBuildTool BuildTool) const;

	UFUNCTION(BlueprintCallable, Category="Building Blueprints")
	bool GrantBlueprint(ESTPBuildTool BuildTool);

	/** Grants a catalog blueprint by its stable BlueprintId. Intended for rewards, trade and discoveries. */
	UFUNCTION(BlueprintCallable, Category="Building Blueprints")
	bool GrantBlueprintById(FName BlueprintId);

	UFUNCTION(BlueprintPure, Category="Building Blueprints")
	bool OwnsBlueprintById(FName BlueprintId) const;

	UFUNCTION(BlueprintCallable, Category="Building Blueprints")
	void RevokeBlueprint(ESTPBuildTool BuildTool);

	UFUNCTION(BlueprintPure, Category="Building Blueprints")
	TArray<ESTPBuildTool> GetOwnedBlueprints() const;

	UPROPERTY(BlueprintAssignable, Category="Building Blueprints")
	FBuildingBlueprintInventoryChanged OnInventoryChanged;

private:
	const class UBuildingDataAsset* FindDefinitionByBlueprintId(FName BlueprintId) const;

	UPROPERTY(SaveGame)
	TSet<ESTPBuildTool> ExplicitlyOwnedBlueprints;
};
