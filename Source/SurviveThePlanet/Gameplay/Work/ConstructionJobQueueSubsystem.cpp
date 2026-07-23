#include "ConstructionJobQueueSubsystem.h"

#include "Gameplay/Base/BaseBuilding.h"

namespace
{
bool IsJobValid(const FSTPConstructionJob& Job)
{
	return IsValid(Job.TargetBuilding);
}
}

void UConstructionJobQueueSubsystem::EnqueueConstructionJob(ABaseBuilding* Building)
{
	if (!IsValid(Building))
	{
		return;
	}

	FSTPConstructionJob Job;
	Job.TargetBuilding = Building;

	Jobs.Add(Job);
}

bool UConstructionJobQueueSubsystem::TryDequeueConstructionJob(FSTPConstructionJob& OutJob)
{
	while (Jobs.Num() > 0)
	{
		OutJob = Jobs[0];
		Jobs.RemoveAt(0);

		if (IsJobValid(OutJob))
		{
			return true;
		}
	}

	return false;
}