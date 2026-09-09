#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Base/BuildingDataAsset.h"

bool UBuildingBlueprintSubsystem::OwnsBlueprint(ESTPBuildTool BuildTool) const
{
	if (BuildTool == ESTPBuildTool::EnergyCable) return true;
	if (ExplicitlyOwnedBlueprints.Contains(BuildTool)) return true;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	const UBuildingDataAsset* Definition = Manager ? Manager->GetDefinition(BuildTool) : nullptr;
	return Definition && Definition->bBlueprintInitiallyOwned;
}

bool UBuildingBlueprintSubsystem::GrantBlueprint(ESTPBuildTool BuildTool)
{
	if (BuildTool == ESTPBuildTool::None || OwnsBlueprint(BuildTool)) return false;
	ExplicitlyOwnedBlueprints.Add(BuildTool);
	OnInventoryChanged.Broadcast(BuildTool);
	return true;
}

void UBuildingBlueprintSubsystem::RevokeBlueprint(ESTPBuildTool BuildTool)
{
	if (ExplicitlyOwnedBlueprints.Remove(BuildTool) > 0) OnInventoryChanged.Broadcast(BuildTool);
}
