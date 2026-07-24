#include "Gameplay/Drones/ConstructionDrone.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/ConstructionDroneCoordinatorSubsystem.h"
#include "SurviveThePlanet.h"
#include "UObject/ConstructorHelpers.h"

AConstructionDrone::AConstructionDrone()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsSelectable = false;

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

void AConstructionDrone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualScale();
}

void AConstructionDrone::BeginPlay()
{
	Super::BeginPlay();

	if (!DroneTag.IsNone())
	{
		Tags.AddUnique(DroneTag);
	}

	if (!DroneTypeTag.IsNone())
	{
		Tags.AddUnique(DroneTypeTag);
	}

	if (UWorld* World = GetWorld())
	{
		if (UConstructionDroneCoordinatorSubsystem* Coordinator = World->GetSubsystem<UConstructionDroneCoordinatorSubsystem>())
		{
			Coordinator->RegisterConstructionDrone(this);
		}
	}
}

void AConstructionDrone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UConstructionDroneCoordinatorSubsystem* Coordinator = World->GetSubsystem<UConstructionDroneCoordinatorSubsystem>())
		{
			Coordinator->UnregisterConstructionDrone(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool AConstructionDrone::AssignConstructionJob(const FSTPConstructionJob& Job)
{
	if (!IsValid(Job.TargetBuilding))
	{
		return false;
	}

	ClearIdleDestination();

	OngoingConstructionJob = FSTPOngoingConstructionJob();
	OngoingConstructionJob.Job = Job;
	OngoingConstructionJob.Phase = ESTPConstructionJobPhase::TravelToBuilding;
	bHasOngoingConstructionJob = true;

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_CONSTRUCTION_DRONE %s assigned construction job for %s."),
		*GetNameSafe(this),
		*GetNameSafe(OngoingConstructionJob.Job.TargetBuilding));

	return true;
}

void AConstructionDrone::TickOngoingConstructionJob(float DeltaSeconds)
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
	case ESTPConstructionJobPhase::None:
	default:
		ClearOngoingConstructionJob();
		break;
	}
}

void AConstructionDrone::ClearOngoingConstructionJob()
{
	OngoingConstructionJob = FSTPOngoingConstructionJob();
	bHasOngoingConstructionJob = false;
}


void AConstructionDrone::SetIdleDestination(FSTPGridCell Cell, const FVector& WorldLocation)
{
	IdleCell = Cell;
	IdleDestination = WorldLocation;
	bHasIdleDestination = true;
}

void AConstructionDrone::ClearIdleDestination()
{
	IdleCell = FSTPGridCell();
	IdleDestination = FVector::ZeroVector;
	bHasIdleDestination = false;
}

void AConstructionDrone::TickIdleMovement(float DeltaSeconds)
{
	if (!bHasIdleDestination || bHasOngoingConstructionJob)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const float Distance = FVector::Dist(CurrentLocation, IdleDestination);
	if (Distance <= WorkRange)
	{
		return;
	}

	const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, IdleDestination, DeltaSeconds, MoveSpeed);
	SetActorLocation(NewLocation);
}
void AConstructionDrone::ConfigureMesh()
{
	DroneMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DroneMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DroneMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	DroneMesh->SetGenerateOverlapEvents(false);
}

void AConstructionDrone::ApplyVisualScale()
{
	if (DroneMesh)
	{
		DroneMesh->SetRelativeScale3D(FVector(DroneVisualScale));
	}
}

bool AConstructionDrone::IsOngoingConstructionJobValid() const
{
	return bHasOngoingConstructionJob && IsValid(OngoingConstructionJob.Job.TargetBuilding);
}

void AConstructionDrone::TickTravelToBuilding(float DeltaSeconds)
{
	ABaseBuilding* TargetBuilding = OngoingConstructionJob.Job.TargetBuilding;
	if (!IsValid(TargetBuilding))
	{
		ClearOngoingConstructionJob();
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetBuilding->GetActorLocation();
	const float Distance = FVector::Dist(CurrentLocation, TargetLocation);
	if (Distance <= WorkRange)
	{
		OngoingConstructionJob.Phase = ESTPConstructionJobPhase::ConstructBuilding;
		return;
	}

	const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaSeconds, MoveSpeed);
	SetActorLocation(NewLocation);
}

void AConstructionDrone::TickBuildTarget(float DeltaSeconds)
{
	ABaseBuilding* TargetBuilding = OngoingConstructionJob.Job.TargetBuilding;
	if (!IsValid(TargetBuilding))
	{
		ClearOngoingConstructionJob();
		return;
	}

	const float NewProgress = TargetBuilding->GetConstructionProgress() + (WorkSpeed * DeltaSeconds);
	TargetBuilding->SetConstructionProgress(NewProgress);

	if (TargetBuilding->GetConstructionProgress() >= 1.0f)
	{
		TargetBuilding->HideConstructionProgress();
		ClearOngoingConstructionJob();
	}
}