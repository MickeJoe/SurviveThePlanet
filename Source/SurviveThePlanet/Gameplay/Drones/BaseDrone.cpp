#include "Gameplay/Drones/BaseDrone.h"

#include "Gameplay/Base/BaseBuilding.h"

ABaseDrone::ABaseDrone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	WorkingRates.Add(ESTPDroneWorkType::Construction, 0.0f);
	WorkingRates.Add(ESTPDroneWorkType::Mining, 0.0f);
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

	SetActorTickEnabled(IsAssignedToBuilding() && !bParkedAtAssignedBuilding);
}

void ABaseDrone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnassignFromBuilding();
	Super::EndPlay(EndPlayReason);
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
	return !IsAssignedToBuilding();
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
