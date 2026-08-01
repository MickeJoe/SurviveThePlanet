#include "Gameplay/Buildings/EnergyStorageBuilding.h"

AEnergyStorageBuilding::AEnergyStorageBuilding()
{
	BuildingTag = TEXT("EnergyStorage");
	BuildingType = ESTPBuildingType::EnergyStorage;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultEnergyStorageBuildingName", "Energy Storage");
	EnergyStorageCapacity = 500.0f;
	EnergyConsumptionPerMinute = 0.0f;
	EnergyProductionPerMinute = 0.0f;
	ConstructionProgress = 0.0f;
}

void AEnergyStorageBuilding::SetPlacementPreview(bool bPreview)
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

void AEnergyStorageBuilding::SetPlacementPreviewValid(bool bValidPlacement)
{
	bPlacementPreviewValid = bValidPlacement;
	RefreshPlacementPreviewVisual();
}

void AEnergyStorageBuilding::RefreshPlacementPreviewVisual()
{
	if (BuildingMesh)
	{
		BuildingMesh->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}
