#include "Gameplay/Buildings/NuclearReactor.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"
ANuclearReactor::ANuclearReactor()
{
 BuildingTag = TEXT("NuclearReactor");
 MaxDroneSlots = 4;
 InitiallyUnlockedDroneSlots = 4;
 DroneWorkType = ESTPDroneWorkType::Construction;
 EnergyProductionPerMinute = 300.0f;
 Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
}
void ANuclearReactor::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 const float Efficiency = GetCombinedDroneEfficiency();
 const TArray<FResourceCost> Inputs = {
  {EResourceType::FuelRods, FMath::Max(1, FuelRodsPerCycle)},
  {EResourceType::CoolingUnits, FMath::Max(1, CoolingUnitsPerCycle)}};
 // Electricity is supplied through the grid; the shared component pays only fuel/coolant.
 bIsProducing = Production->UpdateProduction(DeltaSeconds * Efficiency,
  !IsPlacementPreview() && CanProduce() && Efficiency > 0.0f,
  Inputs, EResourceType::Energy, 0, FMath::Max(0.1f, CycleSeconds));
}
float ANuclearReactor::GetEnergyProductionPerMinute() const
{
 return bIsProducing ? Super::GetEnergyProductionPerMinute() * GetCombinedDroneEfficiency() : 0.0f;
}
