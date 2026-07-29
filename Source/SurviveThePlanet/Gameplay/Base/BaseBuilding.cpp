#include "Gameplay/Base/BaseBuilding.h"

#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Drones/BaseDrone.h"
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
	if (BuildingMesh && BaseModuleMesh)
	{
		BuildingMesh->SetStaticMesh(BaseModuleMesh);
	}
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	UnlockedDroneSlots = FMath::Clamp(InitiallyUnlockedDroneSlots, 0, MaxDroneSlots);
	AssignedDrones.SetNum(MaxDroneSlots);

	if (!BuildingTag.IsNone())
	{
		Tags.AddUnique(BuildingTag);
	}

	RefreshConstructionProgressBar();
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

bool ABaseBuilding::TryAssignDrone(ABaseDrone* Drone, int32 PreferredSlot)
{
	if (!IsValid(Drone) || !Drone->IsAvailableForAssignment() || UnlockedDroneSlots <= 0)
	{
		return false;
	}

	if (AssignedDrones.Num() != MaxDroneSlots)
	{
		AssignedDrones.SetNum(MaxDroneSlots);
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
	const int32 ClampedSlots = FMath::Clamp(NewUnlockedSlots, 0, MaxDroneSlots);
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
