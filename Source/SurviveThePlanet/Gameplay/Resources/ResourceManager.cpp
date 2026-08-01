#include "ResourceManager.h"

AResourceManager::AResourceManager()
{
	PrimaryActorTick.bCanEverTick = false;

	InitialResourceAmounts.Add(EResourceType::Energy, 0);
	InitialResourceAmounts.Add(EResourceType::Iron, 0);
	InitialResourceAmounts.Add(EResourceType::ControlChip, 0);
	InitialResourceAmounts.Add(EResourceType::Copper, 0);
	InitialResourceAmounts.Add(EResourceType::Stone, 0);
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
	const int32 Maximum = ResourceType == EResourceType::Energy ? EnergyStorageCapacity : MAX_int32;
	const int32 ClampedAmount = FMath::Clamp(NewAmount, 0, Maximum);
	int32& CurrentAmount = ResourceAmounts.FindOrAdd(ResourceType);
	if (CurrentAmount == ClampedAmount)
	{
		return;
	}

	CurrentAmount = ClampedAmount;
	OnResourceAmountChanged.Broadcast(ResourceType, CurrentAmount);
}

void AResourceManager::SetEnergyStorageCapacity(int32 NewCapacity)
{
	EnergyStorageCapacity = FMath::Max(0, NewCapacity);
	SetResourceAmount(EResourceType::Energy, GetResourceAmount(EResourceType::Energy));
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

bool AResourceManager::CanAffordCosts(const TArray<FResourceCost>& Costs) const
{
	TMap<EResourceType, int32> CombinedCosts;
	for (const FResourceCost& Entry : Costs)
	{
		if (Entry.Cost < 0)
		{
			return false;
		}

		int32& CombinedCost = CombinedCosts.FindOrAdd(Entry.Resource);
		if (Entry.Cost > MAX_int32 - CombinedCost)
		{
			return false;
		}

		CombinedCost += Entry.Cost;
	}

	for (const TPair<EResourceType, int32>& Entry : CombinedCosts)
	{
		if (!HasResource(Entry.Key, Entry.Value))
		{
			return false;
		}
	}

	return true;
}

bool AResourceManager::TrySpendCosts(const TArray<FResourceCost>& Costs)
{
	if (!CanAffordCosts(Costs))
	{
		return false;
	}

	TMap<EResourceType, int32> CombinedCosts;
	for (const FResourceCost& Entry : Costs)
	{
		CombinedCosts.FindOrAdd(Entry.Resource) += Entry.Cost;
	}

	for (const TPair<EResourceType, int32>& Entry : CombinedCosts)
	{
		if (Entry.Value > 0)
		{
			SetResourceAmount(Entry.Key, GetResourceAmount(Entry.Key) - Entry.Value);
		}
	}

	return true;
}
