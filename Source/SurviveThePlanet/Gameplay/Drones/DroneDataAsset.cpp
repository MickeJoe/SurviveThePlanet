#include "Gameplay/Drones/DroneDataAsset.h"

UDroneDataAsset::UDroneDataAsset()
{
	WorkingRates.Add(ESTPDroneWorkType::Construction, 0.0f);
	WorkingRates.Add(ESTPDroneWorkType::Mining, 0.0f);
}

float UDroneDataAsset::GetWorkingRate(ESTPDroneWorkType WorkType) const
{
	if (const float* Rate = WorkingRates.Find(WorkType))
	{
		return FMath::Max(0.0f, *Rate);
	}

	return 0.0f;
}
