#include "Gameplay/Drones/ConstructionDroneCoordinatorSubsystem.h"

#include "Gameplay/Drones/ConstructionDrone.h"
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

	for (AConstructionDrone* Drone : ConstructionDrones)
	{
		if (!IsValid(Drone))
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
		}
	}
}

TStatId UConstructionDroneCoordinatorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UConstructionDroneCoordinatorSubsystem, STATGROUP_Tickables);
}

void UConstructionDroneCoordinatorSubsystem::RegisterConstructionDrone(AConstructionDrone* Drone)
{
	if (IsValid(Drone))
	{
		ConstructionDrones.AddUnique(Drone);
	}
}

void UConstructionDroneCoordinatorSubsystem::UnregisterConstructionDrone(AConstructionDrone* Drone)
{
	ConstructionDrones.Remove(Drone);
}

void UConstructionDroneCoordinatorSubsystem::RemoveInvalidDrones()
{
	ConstructionDrones.RemoveAll([](const TObjectPtr<AConstructionDrone>& Drone)
	{
		return !IsValid(Drone);
	});
}