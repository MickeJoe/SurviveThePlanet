#include "ResourceManager.h"

AResourceManager::AResourceManager()
{
	PrimaryActorTick.bCanEverTick = false;

	InitialResourceAmounts.Add(EResourceType::Energy, 0);
	InitialResourceAmounts.Add(EResourceType::Iron, 0);
	InitialResourceAmounts.Add(EResourceType::ControlChip, 0);
}

void AResourceManager::BeginPlay()
{
	Super::BeginPlay();

	ResourceAmounts = InitialResourceAmounts;

	for (TPair<EResourceType, int32>& Resource : ResourceAmounts)
	{
		Resource.Value = FMath::Max(0, Resource.Value);
	}
}

int32 AResourceManager::GetResourceAmount(EResourceType ResourceType) const
{
	return ResourceAmounts.FindRef(ResourceType);
}

void AResourceManager::SetResourceAmount(EResourceType ResourceType, int32 NewAmount)
{
	const int32 ClampedAmount = FMath::Max(0, NewAmount);
	int32& CurrentAmount = ResourceAmounts.FindOrAdd(ResourceType);
	if (CurrentAmount == ClampedAmount)
	{
		return;
	}

	CurrentAmount = ClampedAmount;
	OnResourceAmountChanged.Broadcast(ResourceType, CurrentAmount);
}

void AResourceManager::AddResource(EResourceType ResourceType, int32 Amount)
{
	const int32 CurrentAmount = GetResourceAmount(ResourceType);
	SetResourceAmount(ResourceType, CurrentAmount + Amount);
}

bool AResourceManager::HasResource(EResourceType ResourceType, int32 Amount) const
{
	return Amount >= 0 && GetResourceAmount(ResourceType) >= Amount;
}

bool AResourceManager::TrySpendResource(EResourceType ResourceType, int32 Amount)
{
	if (!HasResource(ResourceType, Amount))
	{
		return false;
	}

	SetResourceAmount(ResourceType, GetResourceAmount(ResourceType) - Amount);
	return true;
}
