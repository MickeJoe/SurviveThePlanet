#include "Gameplay/Drones/ConstructionDroneCoordinatorSubsystem.h"

#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Work/ConstructionJobQueueSubsystem.h"
#include "Stats/Stats.h"

void UConstructionDroneCoordinatorSubsystem::Tick(float DeltaTime)
{
	RemoveInvalidDrones();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UConstructionJobQueueSubsystem* JobQueue = World->GetSubsystem<UConstructionJobQueueSubsystem>();
	if (!JobQueue)
	{
		return;
	}

	for (ABaseDrone* Drone : Drones)
	{
		if (!IsValid(Drone))
		{
			continue;
		}

		if (!Drone->CanPerformWork(ESTPDroneWorkType::Construction))
		{
			continue;
		}

		if (Drone->IsAssignedToBuilding())
		{
			continue;
		}

		if (Drone->HasOngoingConstructionJob())
		{
			Drone->TickOngoingConstructionJob(DeltaTime);
			continue;
		}

		FSTPConstructionJob Job;
		if (JobQueue->TryDequeueConstructionJob(Job))
		{
			Drone->AssignConstructionJob(Job);
			continue;
		}

		EnsureDroneHasIdleDestination(Drone);
		Drone->TickIdleMovement(DeltaTime);
	}
}

TStatId UConstructionDroneCoordinatorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UConstructionDroneCoordinatorSubsystem, STATGROUP_Tickables);
}

void UConstructionDroneCoordinatorSubsystem::RegisterDrone(ABaseDrone* Drone)
{
	if (IsValid(Drone))
	{
		Drones.AddUnique(Drone);
	}
}

void UConstructionDroneCoordinatorSubsystem::UnregisterDrone(ABaseDrone* Drone)
{
	Drones.Remove(Drone);
}

void UConstructionDroneCoordinatorSubsystem::RemoveInvalidDrones()
{
	Drones.RemoveAll([](const TObjectPtr<ABaseDrone>& Drone)
	{
		return !IsValid(Drone);
	});
}

void UConstructionDroneCoordinatorSubsystem::EnsureDroneHasIdleDestination(ABaseDrone* Drone)
{
	if (!IsValid(Drone) || Drone->HasIdleDestination())
	{
		return;
	}

	ABaseBuilding* BaseModule = FindBaseModule();
	APlanetSurfaceManager* SurfaceManager = FindPlanetSurfaceManager();
	if (!BaseModule || !SurfaceManager)
	{
		return;
	}

	FSTPGridCell BaseOriginCell;
	if (!SurfaceManager->TryGetActorOriginCell(BaseModule, BaseOriginCell))
	{
		return;
	}

	FSTPGridCell IdleCell;
	FVector IdleWorldLocation = FVector::ZeroVector;
	if (SurfaceManager->FindNearestFreeCellAdjacentToFootprint(BaseOriginCell, BaseModule->GetGridFootprint(), Drone->GetGridFootprint(), IdleCell, IdleWorldLocation))
	{
		Drone->SetIdleDestination(IdleCell, IdleWorldLocation);
	}
}

ABaseBuilding* UConstructionDroneCoordinatorSubsystem::FindBaseModule() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ABaseBuilding> It(World); It; ++It)
	{
		ABaseBuilding* Building = *It;
		if (IsValid(Building) && Building->GetBuildingType() == ESTPBuildingType::BaseModule)
		{
			return Building;
		}
	}

	return nullptr;
}

APlanetSurfaceManager* UConstructionDroneCoordinatorSubsystem::FindPlanetSurfaceManager() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}
