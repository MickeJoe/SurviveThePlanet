#include "Gameplay/Drones/BaseDrone.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/ConstructionDroneCoordinatorSubsystem.h"
#include "SurviveThePlanet.h"
#include "UObject/ConstructorHelpers.h"

ABaseDrone::ABaseDrone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	WorkingRates.Add(ESTPDroneWorkType::Construction, 0.1f);
	WorkingRates.Add(ESTPDroneWorkType::Mining, 0.0f);
	WorkingRates.Add(ESTPDroneWorkType::EnergyProduction, 0.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DroneMeshAsset(TEXT("/Game/Models/Units/WorkingDrone/ConstructionDrone.ConstructionDrone"));
	if (DroneMeshAsset.Succeeded())
	{
		DroneMesh->SetStaticMesh(DroneMeshAsset.Object);
	}

	ApplyVisualScale();
	ConfigureMesh();
}

void ABaseDrone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualScale();
}

void ABaseDrone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsValid(AssignedBuilding) || bParkedAtAssignedBuilding)
	{
		SetActorTickEnabled(false);
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = AssignedBuilding->GetActorLocation();
	if (FVector::Dist(CurrentLocation, TargetLocation) <= AssignmentArrivalDistance)
	{
		ParkAtAssignedBuilding();
		return;
	}

	SetActorLocation(FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaSeconds, AssignmentMoveSpeed));
}

void ABaseDrone::BeginPlay()
{
	Super::BeginPlay();

	if (!DroneTag.IsNone())
	{
		Tags.AddUnique(DroneTag);
	}

	if (UWorld* World = GetWorld())
	{
		if (UConstructionDroneCoordinatorSubsystem* Coordinator = World->GetSubsystem<UConstructionDroneCoordinatorSubsystem>())
		{
			Coordinator->RegisterDrone(this);
		}
	}

	SetActorTickEnabled(IsAssignedToBuilding() && !bParkedAtAssignedBuilding);
}

void ABaseDrone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UConstructionDroneCoordinatorSubsystem* Coordinator = World->GetSubsystem<UConstructionDroneCoordinatorSubsystem>())
		{
			Coordinator->UnregisterDrone(this);
		}
	}

	ClearOngoingConstructionJob();
	UnassignFromBuilding();
	Super::EndPlay(EndPlayReason);
}

FIntPoint ABaseDrone::GetGridFootprint() const
{
	if (!DroneMesh || !DroneMesh->GetStaticMesh())
	{
		return FIntPoint(1, 1);
	}

	const FVector Size = DroneMesh->GetStaticMesh()->GetBoundingBox()
		.TransformBy(DroneMesh->GetRelativeTransform()).GetSize().GetAbs();
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

FText ABaseDrone::GetDroneDisplayName() const
{
	return IsValid(DroneData) ? DroneData->DisplayName : DroneDisplayName;
}

UTexture2D* ABaseDrone::GetDroneThumbnail() const
{
	return IsValid(DroneData) ? DroneData->Thumbnail.Get() : DroneThumbnail.Get();
}

float ABaseDrone::GetWorkingRate(ESTPDroneWorkType WorkType) const
{
	if (IsValid(DroneData))
	{
		return DroneData->GetWorkingRate(WorkType);
	}

	if (const float* Rate = WorkingRates.Find(WorkType))
	{
		return FMath::Max(0.0f, *Rate);
	}

	return 0.0f;
}

bool ABaseDrone::CanPerformWork(ESTPDroneWorkType WorkType) const
{
	return GetWorkingRate(WorkType) > 0.0f;
}

bool ABaseDrone::IsAvailableForAssignment() const
{
	return !IsAssignedToBuilding() && !HasOngoingConstructionJob();
}

bool ABaseDrone::AssignConstructionJob(const FSTPConstructionJob& Job)
{
	if (!CanPerformWork(ESTPDroneWorkType::Construction) || !IsAvailableForAssignment() || !IsValid(Job.TargetBuilding))
	{
		return false;
	}

	ClearIdleDestination();
	OngoingConstructionJob = FSTPOngoingConstructionJob();
	OngoingConstructionJob.Job = Job;
	OngoingConstructionJob.Phase = ESTPConstructionJobPhase::TravelToBuilding;
	bHasOngoingConstructionJob = true;

	UE_LOG(LogSurviveThePlanet, Log, TEXT("STP_DRONE %s assigned construction job for %s."),
		*GetNameSafe(this), *GetNameSafe(Job.TargetBuilding));
	return true;
}

void ABaseDrone::TickOngoingConstructionJob(float DeltaSeconds)
{
	if (!bHasOngoingConstructionJob)
	{
		return;
	}

	if (!IsOngoingConstructionJobValid())
	{
		ClearOngoingConstructionJob();
		return;
	}

	switch (OngoingConstructionJob.Phase)
	{
	case ESTPConstructionJobPhase::TravelToBuilding:
		TickTravelToBuilding(DeltaSeconds);
		break;
	case ESTPConstructionJobPhase::ConstructBuilding:
		TickBuildTarget(DeltaSeconds);
		break;
	default:
		ClearOngoingConstructionJob();
		break;
	}
}

void ABaseDrone::ClearOngoingConstructionJob()
{
	OngoingConstructionJob = FSTPOngoingConstructionJob();
	bHasOngoingConstructionJob = false;
}

void ABaseDrone::SetIdleDestination(FSTPGridCell Cell, const FVector& WorldLocation)
{
	IdleCell = Cell;
	IdleDestination = WorldLocation;
	IdlePath.Reset();
	IdlePathIndex = 0;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			It->FindGridPath(GetActorLocation(), Cell, GetGridFootprint(), IdlePath);
			break;
		}
	}
	bHasIdleDestination = true;
}

void ABaseDrone::ClearIdleDestination()
{
	IdleCell = FSTPGridCell();
	IdleDestination = FVector::ZeroVector;
	IdlePath.Reset();
	IdlePathIndex = 0;
	bHasIdleDestination = false;
}

void ABaseDrone::TickIdleMovement(float DeltaSeconds)
{
	if (!bHasIdleDestination || bHasOngoingConstructionJob || IsAssignedToBuilding())
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (IdlePath.IsValidIndex(IdlePathIndex))
	{
		const FVector Waypoint = IdlePath[IdlePathIndex];
		if (FVector::DistSquared2D(CurrentLocation, Waypoint) <= FMath::Square(10.0f))
		{
			++IdlePathIndex;
			if (!IdlePath.IsValidIndex(IdlePathIndex))
			{
				return;
			}
		}
		SetActorLocation(FMath::VInterpConstantTo(CurrentLocation, IdlePath[IdlePathIndex], DeltaSeconds, MoveSpeed));
		return;
	}

	if (FVector::Dist(CurrentLocation, IdleDestination) <= WorkRange)
	{
		return;
	}

	SetActorLocation(FMath::VInterpConstantTo(CurrentLocation, IdleDestination, DeltaSeconds, MoveSpeed));
}

bool ABaseDrone::AssignToBuilding(ABaseBuilding* Building, int32 PreferredSlot)
{
	return IsValid(Building) && Building->TryAssignDrone(this, PreferredSlot);
}

bool ABaseDrone::UnassignFromBuilding()
{
	if (!IsValid(AssignedBuilding))
	{
		SetBuildingAssignmentInternal(nullptr, INDEX_NONE);
		return false;
	}

	return AssignedBuilding->UnassignDrone(this);
}

void ABaseDrone::SetBuildingAssignmentInternal(ABaseBuilding* Building, int32 SlotIndex)
{
	if (AssignedBuilding == Building && AssignedBuildingSlot == SlotIndex)
	{
		return;
	}

	const bool bIsAssigned = Building != nullptr;
	AssignedBuilding = Building;
	AssignedBuildingSlot = Building ? SlotIndex : INDEX_NONE;
	bParkedAtAssignedBuilding = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(bIsAssigned);
	HandleBuildingAssignmentChanged(bIsAssigned);
	OnBuildingAssignmentChanged.Broadcast(AssignedBuilding);
}

void ABaseDrone::HandleBuildingAssignmentChanged(bool bIsAssigned)
{
	ClearIdleDestination();
}

void ABaseDrone::ParkAtAssignedBuilding()
{
	if (!IsValid(AssignedBuilding))
	{
		return;
	}

	bParkedAtAssignedBuilding = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ABaseDrone::ConfigureMesh()
{
	if (!DroneMesh)
	{
		return;
	}

	DroneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DroneMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DroneMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	DroneMesh->SetGenerateOverlapEvents(false);
}

void ABaseDrone::ApplyVisualScale()
{
	if (DroneMesh)
	{
		DroneMesh->SetRelativeScale3D(FVector(DroneVisualScale));
	}
}

bool ABaseDrone::IsOngoingConstructionJobValid() const
{
	return bHasOngoingConstructionJob && IsValid(OngoingConstructionJob.Job.TargetBuilding);
}

void ABaseDrone::TickTravelToBuilding(float DeltaSeconds)
{
	ABaseBuilding* TargetBuilding = OngoingConstructionJob.Job.TargetBuilding;
	if (!IsValid(TargetBuilding))
	{
		ClearOngoingConstructionJob();
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = TargetBuilding->GetActorLocation();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			FSTPGridCell InteractionCell;
			FVector InteractionLocation;
			if (It->FindNearestFreeCellAdjacentToActor(
				TargetBuilding, GetGridFootprint(), InteractionCell, InteractionLocation))
			{
				TargetLocation = InteractionLocation;
			}
			break;
		}
	}
	if (FVector::Dist(CurrentLocation, TargetLocation) <= WorkRange)
	{
		OngoingConstructionJob.Phase = ESTPConstructionJobPhase::ConstructBuilding;
		return;
	}

	SetActorLocation(FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaSeconds, MoveSpeed));
}

void ABaseDrone::TickBuildTarget(float DeltaSeconds)
{
	ABaseBuilding* TargetBuilding = OngoingConstructionJob.Job.TargetBuilding;
	if (!IsValid(TargetBuilding))
	{
		ClearOngoingConstructionJob();
		return;
	}

	const float NewProgress = TargetBuilding->GetConstructionProgress()
		+ (GetWorkingRate(ESTPDroneWorkType::Construction) * DeltaSeconds);
	TargetBuilding->SetConstructionProgress(NewProgress);

	if (TargetBuilding->GetConstructionProgress() >= 1.0f)
	{
		TargetBuilding->HideConstructionProgress();
		ClearOngoingConstructionJob();
	}
}
