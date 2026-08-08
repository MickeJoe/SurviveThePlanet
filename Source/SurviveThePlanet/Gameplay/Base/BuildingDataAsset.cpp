#include "Gameplay/Base/BuildingDataAsset.h"

UEnergyStorageBuildingDataAsset::UEnergyStorageBuildingDataAsset()
{
	BuildingType = ESTPBuildingType::EnergyStorage;
	BuildingTag = TEXT("EnergyStorage");
	DisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultEnergyStorageName", "Energy Storage");
	EnergyStorageCapacity = 500.0f;
	bOverrideEnergySettings = true;
}

UWaterCollectorBuildingDataAsset::UWaterCollectorBuildingDataAsset()
{
	BuildingType = ESTPBuildingType::WaterCollector;
	BuildingTag = TEXT("WaterCollector");
	DisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultWaterCollectorDataName", "Water Collector");
	EnergyConsumptionPerMinute = 5.0f;
	EnergyProductionPerMinute = 0.0f;
	EnergyStorageCapacity = 0.0f;
	bOverrideEnergySettings = true;
}

UConcretePlantBuildingDataAsset::UConcretePlantBuildingDataAsset()
{
	BuildingType = ESTPBuildingType::ConcretePlant;
	BuildingTag = TEXT("ConcretePlant");
	DisplayName = NSLOCTEXT("SurviveThePlanet", "ConcretePlantDataName", "Concrete Plant");
	EnergyConsumptionPerMinute = 10.0f;
	bOverrideEnergySettings = true;
}
