#include "ThermalProcessingBuilding.h"
#include "ResourceProductionComponent.h"
#include "IndustrialEffectsComponent.h"
#include "Components/StaticMeshComponent.h"

AThermalProcessingBuilding::AThermalProcessingBuilding()
{
 MaxDroneSlots = 4;
 InitiallyUnlockedDroneSlots = 4;
 DroneWorkType = ESTPDroneWorkType::Construction;
 EnergyConsumptionPerMinute = 12.0f;
 Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
 MotionFan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MotionFan"));
 MotionFan->SetupAttachment(GetRootComponent());
 MotionFan->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 IndustrialEffects = CreateDefaultSubobject<UIndustrialEffectsComponent>(TEXT("IndustrialEffects"));
 IndustrialEffects->EffectKind = EIndustrialEffectKind::WaterTreatment;
}

TArray<FResourceCost> AThermalProcessingBuilding::GetRecipeInputs() const
{
 if (Recipe == EThermalRecipe::CoolingUnits)
  return {{EResourceType::Steel, FMath::Max(1, PrimaryInputPerCycle)},
   {EResourceType::PurifiedWater, FMath::Max(1, PurifiedWaterPerCycle)}};
 return {{EResourceType::Water, FMath::Max(1, PrimaryInputPerCycle)}};
}

EResourceType AThermalProcessingBuilding::GetRecipeOutput() const
{
 return Recipe == EThermalRecipe::CoolingUnits ? EResourceType::CoolingUnits : EResourceType::PurifiedWater;
}

void AThermalProcessingBuilding::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 const float Efficiency = GetCombinedDroneEfficiency();
 bIsProducing = Production->UpdateProduction(DeltaSeconds * Efficiency,
  !IsPlacementPreview() && IsOperational() && Efficiency > 0.0f,
  GetRecipeInputs(), GetRecipeOutput(), FMath::Max(1, OutputPerCycle), FMath::Max(0.1f, CycleSeconds));
}
