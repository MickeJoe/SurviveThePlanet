#include "Gameplay/Buildings/CrudeWorkshop.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"
#include "Gameplay/Buildings/IndustrialEffectsComponent.h"
#include "Components/StaticMeshComponent.h"

ACrudeWorkshop::ACrudeWorkshop()
{
	BuildingType = ESTPBuildingType::Other;
	BuildingTag = TEXT("CrudeWorkshop");
	BuildingDisplayName = NSLOCTEXT("STP", "CrudeWorkshopName", "Crude Workshop");
	EnergyConsumptionPerMinute = 6.0f;
	ConstructionProgress = 0.0f;
	bGroundMeshToSurface = true;
	MaxDroneSlots = 4;
	InitiallyUnlockedDroneSlots = 4;
	DroneWorkType = ESTPDroneWorkType::Construction;
	bProductionEnabled = false;
	Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
	Effects = CreateDefaultSubobject<UIndustrialEffectsComponent>(TEXT("Effects"));
	Effects->EffectKind = EIndustrialEffectKind::CrudeWorkshop;
	WorkshopFan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkshopFan"));
	WorkshopRam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkshopRam"));
	for (UStaticMeshComponent* Part : {WorkshopFan.Get(), WorkshopRam.Get()})
	{
		Part->SetupAttachment(BuildingMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
	}
	// FBX mirrors Blender Y; both pivots are local to the body mesh.
	WorkshopFan->SetRelativeLocation(FVector(-82, -64, 226));
	WorkshopRam->SetRelativeLocation(FVector(-62, -32, 121));
}

void ACrudeWorkshop::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	WorkshopFan->SetStaticMesh(FanMesh);
	WorkshopRam->SetStaticMesh(RamMesh);
	GroundBuildingMesh(true);
}

void ACrudeWorkshop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GroundBuildingMesh();
	const float Efficiency = GetCombinedDroneEfficiency();
	bIsProducing = Production->UpdateProduction(DeltaSeconds * Efficiency,
		!bPlacementPreview && CanProduce() && Efficiency > 0.0f, GetRecipeInputs(), GetRecipeOutput(), 1, GetCycleSeconds());
	if (!bIsProducing) return;
	WorkingSeconds += FMath::Max(0.0f, DeltaSeconds);
	WorkshopFan->AddLocalRotation(FRotator(0, DeltaSeconds * 200.0f, 0));
	WorkshopRam->SetRelativeLocation(FVector(-62, -32, 121 - 14 * (0.5f + 0.5f * FMath::Sin(WorkingSeconds * 2.5f))));
}

bool ACrudeWorkshop::SetRecipe(ECrudeWorkshopRecipe Recipe)
{
	if (static_cast<uint8>(Recipe) > static_cast<uint8>(ECrudeWorkshopRecipe::Copper)) return false;
	if (Recipe == SelectedRecipe) return true;
	SelectedRecipe = Recipe;
	// Inputs are paid at completion; switching discards only unfinished work.
	Production->ResetCycle();
	bIsProducing = false;
	return true;
}

TArray<FResourceCost> ACrudeWorkshop::GetRecipeInputs() const
{
	switch (SelectedRecipe)
	{
	case ECrudeWorkshopRecipe::Steel: return {{EResourceType::Iron, FMath::Max(1, IronPerSteel)}};
	case ECrudeWorkshopRecipe::Copper: return {{EResourceType::Copper, FMath::Max(1, OrePerCopper)}};
	default: return {{EResourceType::Stone, FMath::Max(1, StonePerConcrete)}};
	}
}

EResourceType ACrudeWorkshop::GetOutputForRecipe(ECrudeWorkshopRecipe Recipe)
{
	switch (Recipe)
	{
	case ECrudeWorkshopRecipe::Steel: return EResourceType::Steel;
	case ECrudeWorkshopRecipe::Copper: return EResourceType::CopperMetal;
	default: return EResourceType::Concrete;
	}
}

EResourceType ACrudeWorkshop::GetRecipeOutput() const { return GetOutputForRecipe(SelectedRecipe); }

float ACrudeWorkshop::GetCycleSecondsForRecipe(ECrudeWorkshopRecipe Recipe) const
{
	return FMath::Max(0.1f, Recipe == ECrudeWorkshopRecipe::Steel ? SteelCycleSeconds
		: Recipe == ECrudeWorkshopRecipe::Copper ? CopperCycleSeconds : ConcreteCycleSeconds);
}

float ACrudeWorkshop::GetCycleSeconds() const { return GetCycleSecondsForRecipe(SelectedRecipe); }

void ACrudeWorkshop::SetProductionEnabled(bool bEnabled)
{
	Super::SetProductionEnabled(bEnabled);
	if (!bEnabled) { Production->StopProduction(); bIsProducing = false; }
}

void ACrudeWorkshop::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	Production->StopProduction();
	bIsProducing = false;
}
