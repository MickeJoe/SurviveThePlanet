#include "Gameplay/Buildings/PolymerPlant.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"

APolymerPlant::APolymerPlant()
{
	BuildingTag = TEXT("PolymerPlant");
	BuildingType = ESTPBuildingType::PolymerPlant;
	BuildingDisplayName = NSLOCTEXT("STP", "PolymerPlantName", "Polymer Plant");
	BuildingDescription = NSLOCTEXT("STP", "PolymerPlantDescription", "Converts coal and water into polymer while supplied with electricity.");
	EnergyConsumptionPerMinute = 12.0f;
	ConstructionProgress = 0.0f;
	Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
	// GroundMesh resolves the visual offset against terrain after spawning.
	BuildingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -56.55f));
	FanA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanA"));
	FanB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanB"));
	OutputBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OutputBlock"));
	for (UStaticMeshComponent* Part : {FanA.Get(), FanB.Get(), OutputBlock.Get()})
	{
		Part->SetupAttachment(BuildingMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
	}
	FanA->SetRelativeLocation(FVector(55.0f, -65.0f, 216.0f));
	FanB->SetRelativeLocation(FVector(55.0f, 65.0f, 216.0f));
	OutputBlock->SetRelativeLocation(FVector(218.0f, 0.0f, 75.0f));
	ProcessLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ProcessLight"));
	ProcessLight->SetupAttachment(BuildingMesh);
	ProcessLight->SetRelativeLocation(FVector(212.0f, -64.0f, 130.0f));
	ProcessLight->SetLightColor(FLinearColor(0.01f, 0.65f, 1.0f));
	ProcessLight->SetIntensityUnits(ELightUnits::Candelas);
	ProcessLight->SetIntensity(0.0f);
	ProcessLight->SetAttenuationRadius(130.0f);
	ProcessLight->SetCastShadows(false);
}

void APolymerPlant::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FanA->SetStaticMesh(FanMeshAsset);
	FanB->SetStaticMesh(FanMeshAsset);
	OutputBlock->SetStaticMesh(ProductMeshAsset);
	GroundBuildingMesh(true);
	RefreshVisuals();
}

void APolymerPlant::BeginPlay()
{
	Super::BeginPlay();
	GroundBuildingMesh(true);
}

const UPolymerPlantBuildingDataAsset* APolymerPlant::GetRecipe() const
{
	return Cast<UPolymerPlantBuildingDataAsset>(GetBuildingData());
}

void APolymerPlant::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GroundBuildingMesh();
	const UPolymerPlantBuildingDataAsset* Recipe = GetRecipe();
	const TArray<FResourceCost> Inputs = {
		{EResourceType::Coal, FMath::Max(0, Recipe ? Recipe->CoalPerCycle : 2)},
		{EResourceType::Water, FMath::Max(0, Recipe ? Recipe->WaterPerCycle : 1)}
	};
	const float CycleSeconds = FMath::Max(0.1f, Recipe ? Recipe->CycleSeconds : 20.0f);
	const int32 Output = FMath::Max(1, Recipe ? Recipe->PolymerPerCycle : 3);
	bIsProducing = Production->UpdateProduction(DeltaSeconds,
		!bPlacementPreview && IsOperational(), Inputs, EResourceType::Polymer, Output, CycleSeconds);
	if (bIsProducing)
	{
		const float Step = FMath::Max(0.0f, DeltaSeconds);
		VisualTimeSeconds += Step;
		FanA->AddLocalRotation(FRotator(0.0f, 360.0f * Step, 0.0f));
		FanB->AddLocalRotation(FRotator(0.0f, -320.0f * Step, 0.0f));
		// A short extrusion motion stays inside the compact footprint.
		OutputBlock->SetRelativeLocation(FVector(207.0f + 16.0f * FMath::Fmod(VisualTimeSeconds / 3.0f,1.0f),0,75));

	}
	RefreshVisuals();
}

void APolymerPlant::RefreshVisuals()
{
	OutputBlock->SetVisibility(bIsProducing);
	ProcessLight->SetIntensity(bIsProducing ? 12.0f * (0.85f + 0.15f * FMath::Sin(VisualTimeSeconds * 4.0f)) : 0.0f);
	for (UStaticMeshComponent* Part : {FanA.Get(), FanB.Get(), OutputBlock.Get()})
	{
		Part->SetRenderCustomDepth(bPlacementPreview);
		Part->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}

void APolymerPlant::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	Production->StopProduction();
	bIsProducing = false;
	RefreshVisuals();
}

void APolymerPlant::SetPlacementPreviewValid(bool bValidPlacement)
{
	Super::SetPlacementPreviewValid(bValidPlacement);
	RefreshVisuals();
}

float APolymerPlant::GetPolymerProductionPerMinute() const
{
	const auto* R = GetRecipe();
	return (R ? R->PolymerPerCycle : 3) * 60.0f / FMath::Max(0.1f,R ? R->CycleSeconds : 20.0f);
}

float APolymerPlant::GetCoalConsumptionPerMinute() const
{
	const auto* R = GetRecipe();
	return FMath::Max(0,R ? R->CoalPerCycle : 2) * 60.0f / FMath::Max(0.1f,R ? R->CycleSeconds : 20.0f);
}

float APolymerPlant::GetWaterConsumptionPerMinute() const
{
	const auto* R = GetRecipe();
	return FMath::Max(0,R ? R->WaterPerCycle : 1) * 60.0f / FMath::Max(0.1f,R ? R->CycleSeconds : 20.0f);
}

