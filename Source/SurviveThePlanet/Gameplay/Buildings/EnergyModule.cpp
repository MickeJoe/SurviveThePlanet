#include "Gameplay/Buildings/EnergyModule.h"

#include "Components/StaticMeshComponent.h"

AEnergyModule::AEnergyModule()
{
	BuildingTag = TEXT("EnergyModule");
	BuildingType = ESTPBuildingType::EnergyModule;
	DroneWorkType = ESTPDroneWorkType::EnergyProduction;
	EnergyStorageCapacity = 0.0f;
	EnergyProductionPerMinute = 60.0f;
	ConstructionProgress = 0.0f;
}

void AEnergyModule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyModuleMesh();
	RefreshPlacementPreviewVisual();
}

void AEnergyModule::BeginPlay()
{
	Super::BeginPlay();
	ApplyModuleMesh();
}

void AEnergyModule::SetPlacementPreview(bool bPreview)
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

void AEnergyModule::SetPlacementPreviewValid(bool bValidPlacement)
{
	bPlacementPreviewValid = bValidPlacement;
	RefreshPlacementPreviewVisual();
}

void AEnergyModule::ApplyModuleMesh()
{
	if (!IsValid(BuildingData) && BuildingMesh && ModuleMesh)
	{
		BuildingMesh->SetStaticMesh(ModuleMesh);
	}
}

void AEnergyModule::RefreshPlacementPreviewVisual()
{
	if (BuildingMesh)
	{
		BuildingMesh->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}
