#include "Gameplay/Buildings/ResourceProductionComponent.h"

#include "EngineUtils.h"
#include "Engine/World.h"

UResourceProductionComponent::UResourceProductionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UResourceProductionComponent::UpdateProduction(float DeltaSeconds, bool bCanRun,
	const TArray<FResourceCost>& Inputs, EResourceType OutputResource,
	int32 OutputPerCycle, float CycleSeconds)
{
	if (!IsValid(ResourceManager) && GetWorld())
	{
		for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It)
		{
			ResourceManager = *It;
			break;
		}
	}

	bIsProducing = bCanRun && IsValid(ResourceManager) && OutputPerCycle > 0
		&& ResourceManager->CanAffordCosts(Inputs);
	if (!bIsProducing) return false;

	const float Duration = FMath::Max(0.1f, CycleSeconds);
	CycleProgressSeconds += FMath::Max(0.0f, DeltaSeconds);
	while (CycleProgressSeconds >= Duration)
	{
		// Pay every input together so starvation cannot consume only part of a recipe.
		if (!ResourceManager->TrySpendCosts(Inputs))
		{
			bIsProducing = false;
			break;
		}
		ResourceManager->AddResource(OutputResource, OutputPerCycle);
		CycleProgressSeconds -= Duration;
	}
	bIsProducing = ResourceManager->CanAffordCosts(Inputs);
	return bIsProducing;
}
