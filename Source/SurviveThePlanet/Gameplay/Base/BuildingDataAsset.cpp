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

USteelworksBuildingDataAsset::USteelworksBuildingDataAsset()
{
	BuildCategory = ESTPBuildCategory::Industry;
	BuildingType = ESTPBuildingType::Steelworks;
	BuildingTag = TEXT("Steelworks");
	DisplayName = NSLOCTEXT("SurviveThePlanet", "SteelworksDataName", "Steelworks");
	EnergyConsumptionPerMinute = 18.0f;
	bOverrideEnergySettings = true;
}

UPolymerPlantBuildingDataAsset::UPolymerPlantBuildingDataAsset()
{
	BuildCategory = ESTPBuildCategory::Industry;
	BuildTool = ESTPBuildTool::PolymerPlant;
	BuildingType = ESTPBuildingType::PolymerPlant;
	BuildingTag = TEXT("PolymerPlant");
	BlueprintId = TEXT("PolymerPlant");
	DisplayName = NSLOCTEXT("STP", "PolymerPlantName", "Polymer Plant");
	Description = NSLOCTEXT("STP", "PolymerPlantDescription", "Converts coal and water into polymer while supplied with electricity.");
	EnergyConsumptionPerMinute = 12.0f;
	bOverrideEnergySettings = true;
}

UConnectorPlantBuildingDataAsset::UConnectorPlantBuildingDataAsset()
{
	BuildCategory = ESTPBuildCategory::Industry;
	BuildTool = ESTPBuildTool::ConnectorPlant;
	BuildingType = ESTPBuildingType::ConnectorPlant;
	BuildingTag = TEXT("ConnectorPlant");
	BlueprintId = TEXT("ConnectorPlant");
	DisplayName = NSLOCTEXT("STP", "ConnectorPlantName", "Connector Plant");
	Description = NSLOCTEXT("STP", "ConnectorPlantDescription", "Consumes copper, polymer and electricity to produce connectors.");
	EnergyConsumptionPerMinute = 12.0f;
	bOverrideEnergySettings = true;
}
