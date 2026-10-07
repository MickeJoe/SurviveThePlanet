#include "Gameplay/Buildings/ConnectorPlant.h"

#include "Gameplay/Buildings/ResourceProductionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"

AConnectorPlant::AConnectorPlant()
{
	BuildingType = ESTPBuildingType::ConnectorPlant;
	BuildingTag = TEXT("ConnectorPlant");
	BuildingDisplayName = NSLOCTEXT("STP", "ConnectorPlantName", "Connector Plant");
	EnergyConsumptionPerMinute = 12.0f;
	ConstructionProgress = 0.0f;
	BuildingMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	Production = CreateDefaultSubobject<UResourceProductionComponent>(TEXT("Production"));
	FanA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanA"));
	FanB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanB"));
	for (UStaticMeshComponent* Part : {FanA.Get(), FanB.Get()})
	{
		Part->SetupAttachment(BuildingMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
	}
	FanA->SetRelativeLocation(FVector(-82.0f, 38.0f, 244.0f));
	FanB->SetRelativeLocation(FVector(15.0f, 38.0f, 244.0f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SparkMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UStaticMeshComponent* Product = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ConveyorProduct%d"), Index));
		Product->SetupAttachment(BuildingMesh);
		Product->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Product->SetGenerateOverlapEvents(false);
		Product->SetRelativeLocation(FVector(-100.0f + Index * 80.0f, -75.0f, 112.0f));
		ConveyorProducts.Add(Product);
		UStaticMeshComponent* Spark = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("WeldingSpark%d"), Index));
		Spark->SetupAttachment(BuildingMesh);
		Spark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Spark->SetGenerateOverlapEvents(false);
		Spark->SetCastShadow(false);
		Spark->SetStaticMesh(SparkMesh.Object);
		Spark->SetRelativeScale3D(FVector(0.025f));
		Spark->SetVisibility(false);
		WeldingSparks.Add(Spark);
	}
	ProcessLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ProcessLight"));
	ProcessLight->SetupAttachment(BuildingMesh);
	ProcessLight->SetRelativeLocation(FVector(0.0f, -65.0f, 145.0f));
	ProcessLight->SetLightColor(FLinearColor(1.0f, 0.35f, 0.06f));
	ProcessLight->SetIntensityUnits(ELightUnits::Candelas);
	ProcessLight->SetIntensity(0.0f);
	ProcessLight->SetAttenuationRadius(200.0f);
	ProcessLight->SetCastShadows(false);
}

void AConnectorPlant::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FanA->SetStaticMesh(FanMeshA);
	FanB->SetStaticMesh(FanMeshB);
	for (UStaticMeshComponent* Product : ConveyorProducts) Product->SetStaticMesh(ProductMesh);
	for (UStaticMeshComponent* Spark : WeldingSparks) Spark->SetMaterial(0, SparkMaterial);
	GroundBuildingMesh(true);
	RefreshVisuals();
}

void AConnectorPlant::BeginPlay()
{
	Super::BeginPlay();
	GroundBuildingMesh(true);
}

const UConnectorPlantBuildingDataAsset* AConnectorPlant::GetRecipe() const
{
	return Cast<UConnectorPlantBuildingDataAsset>(GetBuildingData());
}

void AConnectorPlant::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GroundBuildingMesh();
	const auto* Recipe = GetRecipe();
	const TArray<FResourceCost> Inputs = {
		{EResourceType::Copper, FMath::Max(0, Recipe ? Recipe->CopperPerCycle : 2)},
		{EResourceType::Polymer, FMath::Max(0, Recipe ? Recipe->PolymerPerCycle : 1)}
	};
	bIsProducing = Production->UpdateProduction(DeltaSeconds,
		!bPlacementPreview && IsOperational(), Inputs, EResourceType::Connector,
		FMath::Max(1, Recipe ? Recipe->ConnectorsPerCycle : 3),
		Recipe ? Recipe->CycleSeconds : 20.0f);
	if (bIsProducing)
	{
		const float Step = FMath::Max(0.0f, DeltaSeconds);
		VisualTimeSeconds += Step;
		FanA->AddLocalRotation(FRotator(0.0f, 250.0f * Step, 0.0f));
		FanB->AddLocalRotation(FRotator(0.0f, -280.0f * Step, 0.0f));
		for (int32 Index = 0; Index < ConveyorProducts.Num(); ++Index)
		{
			const float Travel = FMath::Fmod(VisualTimeSeconds / 6.0f + Index / 3.0f, 1.0f);
			ConveyorProducts[Index]->SetRelativeLocation(FVector(-115.0f + 230.0f * Travel, -75.0f, 112.0f));
		}
	}
	RefreshVisuals();
}

void AConnectorPlant::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	Production->StopProduction();
	bIsProducing = false;
	RefreshVisuals();
}

void AConnectorPlant::SetPlacementPreviewValid(bool bValidPlacement)
{
	Super::SetPlacementPreviewValid(bValidPlacement);
	RefreshVisuals();
}

float AConnectorPlant::GetConnectorProductionPerMinute() const
{
	const auto* R = GetRecipe();
	return FMath::Max(1, R ? R->ConnectorsPerCycle : 3) * 60.0f
		/ FMath::Max(0.1f, R ? R->CycleSeconds : 20.0f);
}
float AConnectorPlant::GetCopperConsumptionPerMinute() const
{
	const auto* R = GetRecipe();
	return FMath::Max(0, R ? R->CopperPerCycle : 2) * 60.0f
		/ FMath::Max(0.1f, R ? R->CycleSeconds : 20.0f);
}
float AConnectorPlant::GetPolymerConsumptionPerMinute() const
{
	const auto* R = GetRecipe();
	return FMath::Max(0, R ? R->PolymerPerCycle : 1) * 60.0f
		/ FMath::Max(0.1f, R ? R->CycleSeconds : 20.0f);
}

void AConnectorPlant::RefreshVisuals()
{
	// Welding bursts repeat independently of inventory cycles to make operation readable.
	const float BurstPhase = FMath::Fmod(VisualTimeSeconds, 1.4f);
	const bool bWelding = bIsProducing && BurstPhase < 0.35f;
	ProcessLight->SetIntensity(bWelding ? 220.0f : 0.0f);
	for (int32 Index = 0; Index < WeldingSparks.Num(); ++Index)
	{
		UStaticMeshComponent* Spark = WeldingSparks[Index];
		const float Flight = FMath::Fmod(BurstPhase + Index * 0.07f, 0.35f) / 0.35f;
		Spark->SetRelativeLocation(FVector(-70.0f + Index * 70.0f + 16.0f * Flight, -76.0f - 20.0f * Flight, 125.0f + 30.0f * Flight - 20.0f * Flight * Flight));
		Spark->SetVisibility(bWelding);
	}
	TArray<UStaticMeshComponent*> Parts = {FanA.Get(), FanB.Get()};
	for (UStaticMeshComponent* Product : ConveyorProducts) Parts.Add(Product);
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetRenderCustomDepth(bPlacementPreview);
		Part->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}
