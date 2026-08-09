#include "Gameplay/Buildings/ConcretePlant.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Gameplay/Resources/ResourceManager.h"

AConcretePlant::AConcretePlant()
{
	BuildingTag = TEXT("ConcretePlant");
	BuildingType = ESTPBuildingType::ConcretePlant;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultConcretePlantName", "Concrete Plant");
	BuildingDescription = NSLOCTEXT("SurviveThePlanet", "DefaultConcretePlantDescription", "Uses water, stone and electricity to produce concrete.");
	EnergyConsumptionPerMinute = 10.0f;
	ConstructionProgress = 0.0f;
}

void AConcretePlant::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPlacementPreview || GetConstructionProgress() < 1.0f || !IsOperational())
	{
		return;
	}

	const UConcretePlantBuildingDataAsset* Data = GetConcretePlantData();
	const float CycleSeconds = Data ? FMath::Max(0.01f, Data->CycleSeconds) : 60.0f;
	CycleProgressSeconds += FMath::Max(0.0f, DeltaSeconds);
	while (CycleProgressSeconds >= CycleSeconds)
	{
		AResourceManager* Manager = nullptr;
		for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
		if (!Manager) { CycleProgressSeconds = 0.0f; return; }
		const int32 Water = FMath::CeilToInt(Data ? Data->WaterPerCycle : 2.0f);
		const int32 Stone = FMath::CeilToInt(Data ? Data->StonePerCycle : 4.0f);
		const int32 Concrete = FMath::FloorToInt(Data ? Data->ConcretePerCycle : 3.0f);
		const TArray<FResourceCost> Inputs = {{EResourceType::Water, Water}, {EResourceType::Stone, Stone}};
		if (!Manager->TrySpendCosts(Inputs)) { CycleProgressSeconds = CycleSeconds; return; }
		Manager->AddResource(EResourceType::Concrete, Concrete);
		CycleProgressSeconds -= CycleSeconds;
	}
}

float AConcretePlant::GetConcreteProductionPerMinute() const
{
	const UConcretePlantBuildingDataAsset* Data = GetConcretePlantData();
	return Data ? Data->ConcretePerCycle * 60.0f / FMath::Max(0.01f, Data->CycleSeconds) : 3.0f;
}

void AConcretePlant::SetPlacementPreview(bool bPreview)
{
	bPlacementPreview = bPreview;
	bIsSelectable = !bPreview;
	SetActorEnableCollision(!bPreview);
	SetConstructionProgress(bPreview ? 1.0f : 0.0f);
	if (BuildingMesh)
	{
		BuildingMesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		BuildingMesh->SetRenderCustomDepth(bPreview);
	}
	RefreshPlacementPreviewVisual();
}

void AConcretePlant::SetPlacementPreviewValid(bool bValidPlacement)
{
	bPlacementPreviewValid = bValidPlacement;
	RefreshPlacementPreviewVisual();
}

void AConcretePlant::RefreshPlacementPreviewVisual()
{
	if (BuildingMesh) BuildingMesh->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
}

const UConcretePlantBuildingDataAsset* AConcretePlant::GetConcretePlantData() const
{
	return Cast<UConcretePlantBuildingDataAsset>(GetBuildingData());
}
