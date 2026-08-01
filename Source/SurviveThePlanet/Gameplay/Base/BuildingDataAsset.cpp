#include "Gameplay/Base/BuildingDataAsset.h"

UEnergyStorageBuildingDataAsset::UEnergyStorageBuildingDataAsset()
{
	BuildingType = ESTPBuildingType::EnergyStorage;
	BuildingTag = TEXT("EnergyStorage");
	DisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultEnergyStorageName", "Energy Storage");
	EnergyStorageCapacity = 500.0f;
	bOverrideEnergySettings = true;
}
