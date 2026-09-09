#include "Gameplay/Cheats/STPCheatManager.h"

#include "EngineUtils.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"

bool USTPCheatManager::GiveResource(EResourceType ResourceType, int32 Amount)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (Amount <= 0 || !GetOuterAPlayerController())
	{
		return false;
	}

	UWorld* World = GetOuterAPlayerController()->GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AResourceManager> It(World); It; ++It)
	{
		It->AddResource(ResourceType, Amount);
		return true;
	}

	return false;
#endif
}

bool USTPCheatManager::GrantBuildingBlueprint(ESTPBuildTool BuildTool)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	APlayerController* PC = GetOuterAPlayerController();
	UGameInstance* GI = PC ? PC->GetGameInstance() : nullptr;
	UBuildingBlueprintSubsystem* Inventory = GI ? GI->GetSubsystem<UBuildingBlueprintSubsystem>() : nullptr;
	return Inventory && Inventory->GrantBlueprint(BuildTool);
#endif
}

bool USTPCheatManager::CompleteObjective(FName ObjectiveId)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	APlayerController* PlayerController = GetOuterAPlayerController();
	UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
	UObjectiveSubsystem* Objectives = World ? World->GetSubsystem<UObjectiveSubsystem>() : nullptr;
	return Objectives && Objectives->DebugCompleteObjective(ObjectiveId);
#endif
}
