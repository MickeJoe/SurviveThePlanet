#include "Gameplay/Base/BaseBuilding.h"

#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/UI/ConstructionProgressBarWidget.h"

ABaseBuilding::ABaseBuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(SceneRoot);

	ConstructionProgressBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConstructionProgressBar"));
	ConstructionProgressBar->SetupAttachment(SceneRoot);
	ConstructionProgressBar->SetWidgetClass(UConstructionProgressBarWidget::StaticClass());
	ConstructionProgressBar->SetWidgetSpace(EWidgetSpace::Screen);
	ConstructionProgressBar->SetDrawSize(FVector2D(120.0f, 14.0f));
	ConstructionProgressBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	ConstructionProgressBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConstructionProgressBar->SetHiddenInGame(false);

	ConfigureMesh();
}

void ABaseBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Blueprints may configure the inherited BuildingMesh component directly.
	// Only apply the optional base-class override when one was explicitly set;
	// assigning nullptr here would erase the Blueprint component template mesh.
	UStaticMesh* ConfiguredMesh = IsValid(BuildingData) ? BuildingData->BuildingMesh.Get() : BaseModuleMesh.Get();
	if (BuildingMesh && ConfiguredMesh)
	{
		BuildingMesh->SetStaticMesh(ConfiguredMesh);
	}
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = GetMaxHealth();
	const int32 SlotCapacity = GetMaxDroneSlots();
	UnlockedDroneSlots = FMath::Clamp(GetInitiallyUnlockedDroneSlots(), 0, SlotCapacity);
	AssignedDrones.SetNum(SlotCapacity);

	const FName ConfiguredTag = GetBuildingTag();
	if (!ConfiguredTag.IsNone())
	{
		Tags.AddUnique(ConfiguredTag);
	}

	RefreshConstructionProgressBar();
}

bool ABaseBuilding::IsConnectedToPowerGrid() const
{
	if (GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return GetConstructionProgress() >= 1.0f;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			return It->IsBuildingConnectedToPowerGrid(this);
		}
	}
	return false;
}

bool ABaseBuilding::IsOperational() const
{
	if (GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return true;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			return It->IsBuildingOperational(this);
		}
	}
	return false;
}

float ABaseBuilding::GetEnergyConsumptionPerMinute() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyConsumptionPerMinute
		: EnergyConsumptionPerMinute;
}

float ABaseBuilding::GetEnergyProductionPerMinute() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyProductionPerMinute
		: EnergyProductionPerMinute;
}

float ABaseBuilding::GetEnergyStorageCapacity() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyStorageCapacity
		: EnergyStorageCapacity;
}

ABaseDrone* ABaseBuilding::GetAssignedDroneAtSlot(int32 SlotIndex) const
{
	return AssignedDrones.IsValidIndex(SlotIndex) ? AssignedDrones[SlotIndex] : nullptr;
}

int32 ABaseBuilding::GetAssignedDroneCount() const
{
	int32 AssignedCount = 0;
	for (const ABaseDrone* Drone : AssignedDrones)
	{
		AssignedCount += IsValid(Drone) ? 1 : 0;
	}

	return AssignedCount;
}

float ABaseBuilding::GetCombinedDroneEfficiency() const
{
	const ESTPDroneWorkType WorkType = GetDroneWorkType();
	float CombinedEfficiency = 0.0f;
	for (const ABaseDrone* Drone : AssignedDrones)
	{
		if (IsValid(Drone))
		{
			CombinedEfficiency += Drone->GetWorkingRate(WorkType);
		}
	}

	return CombinedEfficiency;
}

bool ABaseBuilding::TryAssignDrone(ABaseDrone* Drone, int32 PreferredSlot)
{
	if (!IsValid(Drone) || !Drone->IsAvailableForAssignment() || UnlockedDroneSlots <= 0)
	{
		return false;
	}

	const int32 SlotCapacity = GetMaxDroneSlots();
	if (AssignedDrones.Num() != SlotCapacity)
	{
		AssignedDrones.SetNum(SlotCapacity);
	}

	int32 SlotIndex = PreferredSlot;
	if (SlotIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < UnlockedDroneSlots; ++Index)
		{
			if (!IsValid(AssignedDrones[Index]))
			{
				SlotIndex = Index;
				break;
			}
		}
	}

	if (SlotIndex < 0 || SlotIndex >= UnlockedDroneSlots || !AssignedDrones.IsValidIndex(SlotIndex)
		|| IsValid(AssignedDrones[SlotIndex]))
	{
		return false;
	}

	AssignedDrones[SlotIndex] = Drone;
	Drone->SetBuildingAssignmentInternal(this, SlotIndex);
	OnDroneAssignmentsChanged.Broadcast();
	return true;
}

bool ABaseBuilding::UnassignDrone(ABaseDrone* Drone)
{
	if (!IsValid(Drone))
	{
		return false;
	}

	const int32 SlotIndex = AssignedDrones.IndexOfByKey(Drone);
	return SlotIndex != INDEX_NONE && UnassignDroneAtSlot(SlotIndex);
}

bool ABaseBuilding::UnassignDroneAtSlot(int32 SlotIndex)
{
	if (!AssignedDrones.IsValidIndex(SlotIndex) || !IsValid(AssignedDrones[SlotIndex]))
	{
		return false;
	}

	ABaseDrone* Drone = AssignedDrones[SlotIndex];
	AssignedDrones[SlotIndex] = nullptr;
	Drone->SetBuildingAssignmentInternal(nullptr, INDEX_NONE);
	OnDroneAssignmentsChanged.Broadcast();
	return true;
}

void ABaseBuilding::SetUnlockedDroneSlots(int32 NewUnlockedSlots)
{
	const int32 ClampedSlots = FMath::Clamp(NewUnlockedSlots, 0, GetMaxDroneSlots());
	if (UnlockedDroneSlots == ClampedSlots)
	{
		return;
	}

	for (int32 SlotIndex = AssignedDrones.Num() - 1; SlotIndex >= ClampedSlots; --SlotIndex)
	{
		UnassignDroneAtSlot(SlotIndex);
	}

	UnlockedDroneSlots = ClampedSlots;
	OnDroneSlotsChanged.Broadcast(UnlockedDroneSlots);
}

void ABaseBuilding::UnlockDroneSlots(int32 SlotsToUnlock)
{
	if (SlotsToUnlock > 0)
	{
		SetUnlockedDroneSlots(UnlockedDroneSlots + SlotsToUnlock);
	}
}

void ABaseBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int32 SlotIndex = 0; SlotIndex < AssignedDrones.Num(); ++SlotIndex)
	{
		UnassignDroneAtSlot(SlotIndex);
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			It->ReleaseCells(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABaseBuilding::ConfigureMesh()
{
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BuildingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BuildingMesh->SetGenerateOverlapEvents(false);
}

void ABaseBuilding::SetConstructionProgress(float NewProgress)
{
	ConstructionProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	RefreshConstructionProgressBar();
}

void ABaseBuilding::ShowConstructionProgress()
{
	if (ConstructionProgressBar)
	{
		ConstructionProgressBar->SetHiddenInGame(false);
	}
}

void ABaseBuilding::HideConstructionProgress()
{
	if (ConstructionProgressBar)
	{
		ConstructionProgressBar->SetHiddenInGame(true);
	}
}

void ABaseBuilding::RefreshConstructionProgressBar()
{
	if (!ConstructionProgressBar)
	{
		return;
	}

	ConstructionProgressBar->SetHiddenInGame(ConstructionProgress >= 1.0f);

	if (UConstructionProgressBarWidget* ProgressWidget = Cast<UConstructionProgressBarWidget>(ConstructionProgressBar->GetUserWidgetObject()))
	{
		ProgressWidget->SetProgress(ConstructionProgress);
	}
}

FIntPoint ABaseBuilding::GetGridFootprint() const
{
	const UStaticMesh* Mesh = BuildingMesh ? BuildingMesh->GetStaticMesh() : nullptr;
	if (!Mesh && IsValid(BuildingData))
	{
		Mesh = BuildingData->BuildingMesh.Get();
	}
	if (!Mesh)
	{
		return FIntPoint(1, 1);
	}

	// Convert the imported mesh bounds through the component transform so BP scale,
	// rotation and pivot offsets are included in the occupied grid area.
	const FTransform MeshToActor = BuildingMesh ? BuildingMesh->GetRelativeTransform() : FTransform::Identity;
	const FVector Size = Mesh->GetBoundingBox().TransformBy(MeshToActor).GetSize().GetAbs();
	float CellSize = 100.0f;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			CellSize = FMath::Max(1.0f, It->GetTileSpacing());
			break;
		}
	}

	return FIntPoint(
		FMath::Max(1, FMath::CeilToInt((Size.X - KINDA_SMALL_NUMBER) / CellSize)),
		FMath::Max(1, FMath::CeilToInt((Size.Y - KINDA_SMALL_NUMBER) / CellSize)));
}

ESTPBuildingType ABaseBuilding::GetBuildingType() const
{
	return IsValid(BuildingData) ? BuildingData->BuildingType : BuildingType;
}

FText ABaseBuilding::GetBuildingDisplayName() const
{
	return IsValid(BuildingData) ? BuildingData->DisplayName : BuildingDisplayName;
}

FText ABaseBuilding::GetBuildingDescription() const
{
	return IsValid(BuildingData) ? BuildingData->Description : BuildingDescription;
}

UTexture2D* ABaseBuilding::GetBuildingThumbnail() const
{
	return IsValid(BuildingData) ? BuildingData->Thumbnail.Get() : BuildingThumbnail.Get();
}

int32 ABaseBuilding::GetMaxDroneSlots() const
{
	return FMath::Max(0, IsValid(BuildingData) ? BuildingData->MaxDroneSlots : MaxDroneSlots);
}

int32 ABaseBuilding::GetInitiallyUnlockedDroneSlots() const
{
	return FMath::Max(0, IsValid(BuildingData) ? BuildingData->InitiallyUnlockedDroneSlots : InitiallyUnlockedDroneSlots);
}

ESTPDroneWorkType ABaseBuilding::GetDroneWorkType() const
{
	return IsValid(BuildingData) ? BuildingData->DroneWorkType : DroneWorkType;
}

const TArray<FResourceCost>& ABaseBuilding::GetConstructionCosts() const
{
	return IsValid(BuildingData) ? BuildingData->ConstructionCosts : ConstructionCosts;
}

float ABaseBuilding::GetMaxHealth() const
{
	return FMath::Max(0.0f, IsValid(BuildingData) ? BuildingData->MaxHealth : MaxHealth);
}

FName ABaseBuilding::GetBuildingTag() const
{
	return IsValid(BuildingData) ? BuildingData->BuildingTag : BuildingTag;
}
