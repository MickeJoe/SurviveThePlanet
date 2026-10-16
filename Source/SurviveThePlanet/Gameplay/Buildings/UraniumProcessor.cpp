#include "Gameplay/Buildings/UraniumProcessor.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"

AUraniumProcessor::AUraniumProcessor()
{
 BuildingTag = TEXT("UraniumProcessor");
 BuildingDisplayName = NSLOCTEXT("STP", "UraniumProcessorName", "Uranium Processor");
 MaxDroneSlots = 4;
 InitiallyUnlockedDroneSlots = 4;
 DroneWorkType = ESTPDroneWorkType::Mining;
 Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
}

void AUraniumProcessor::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 const float Efficiency = GetCombinedDroneEfficiency();
 const TArray<FResourceCost> Inputs = {{EResourceType::UraniumOre, FMath::Max(1, OrePerCycle)}};
 bIsProducing = Production->UpdateProduction(DeltaSeconds * Efficiency,
  !IsPlacementPreview() && CanProduce() && Efficiency > 0.0f,
  Inputs, EResourceType::FuelRods, FMath::Max(1, FuelRodsPerCycle), FMath::Max(0.1f, CycleSeconds));
}
