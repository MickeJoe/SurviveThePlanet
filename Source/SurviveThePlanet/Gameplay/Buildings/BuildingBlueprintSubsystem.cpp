#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Base/BuildingDataAsset.h"

bool UBuildingBlueprintSubsystem::OwnsBlueprint(ESTPBuildTool BuildTool) const
{
	if (BuildTool == ESTPBuildTool::EnergyCable) return false;
	if (ExplicitlyOwnedBlueprints.Contains(BuildTool)) return true;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	const UBuildingDataAsset* Definition = Manager ? Manager->GetDefinition(BuildTool) : nullptr;
	return Definition && Definition->bBlueprintInitiallyOwned;
}

bool UBuildingBlueprintSubsystem::GrantBlueprint(ESTPBuildTool BuildTool)
{
	if (BuildTool == ESTPBuildTool::None || BuildTool == ESTPBuildTool::EnergyCable || OwnsBlueprint(BuildTool)) return false;
	ExplicitlyOwnedBlueprints.Add(BuildTool);
	OnInventoryChanged.Broadcast(BuildTool);
	return true;
}

bool UBuildingBlueprintSubsystem::GrantBlueprintById(FName BlueprintId)
{
	const UBuildingDataAsset* Definition = FindDefinitionByBlueprintId(BlueprintId);
	return Definition && GrantBlueprint(Definition->BuildTool);
}

bool UBuildingBlueprintSubsystem::OwnsBlueprintById(FName BlueprintId) const
{
	const UBuildingDataAsset* Definition = FindDefinitionByBlueprintId(BlueprintId);
	return Definition && OwnsBlueprint(Definition->BuildTool);
}

void UBuildingBlueprintSubsystem::RevokeBlueprint(ESTPBuildTool BuildTool)
{
	if (ExplicitlyOwnedBlueprints.Remove(BuildTool) > 0) OnInventoryChanged.Broadcast(BuildTool);
}

TArray<ESTPBuildTool> UBuildingBlueprintSubsystem::GetOwnedBlueprints() const
{
	TArray<ESTPBuildTool> Result;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	if (Manager)
	{
		for (const UBuildingDataAsset* Definition : Manager->GetAllDefinitions())
		{
			if (Definition && OwnsBlueprint(Definition->BuildTool)) Result.AddUnique(Definition->BuildTool);
		}
	}
	return Result;
}

const UBuildingDataAsset* UBuildingBlueprintSubsystem::FindDefinitionByBlueprintId(FName BlueprintId) const
{
	if (BlueprintId.IsNone()) return nullptr;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	if (!Manager) return nullptr;
	for (const UBuildingDataAsset* Definition : Manager->GetAllDefinitions())
	{
		if (!Definition) continue;
		if (Definition->BlueprintId == BlueprintId) return Definition;

		// Existing catalog assets predate BlueprintId. Their build-tool enum name is a
		// stable fallback, so rewards can be authored before every asset is resaved.
		const UEnum* BuildToolEnum = StaticEnum<ESTPBuildTool>();
		if (BuildToolEnum && FName(BuildToolEnum->GetNameStringByValue(static_cast<int64>(Definition->BuildTool))) == BlueprintId)
		{
			return Definition;
		}
	}
	return nullptr;
}
