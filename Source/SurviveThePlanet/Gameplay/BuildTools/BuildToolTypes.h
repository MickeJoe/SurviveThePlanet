#pragma once

#include "CoreMinimal.h"
#include "BuildToolTypes.generated.h"

UENUM(BlueprintType)
enum class ESTPBuildTool : uint8
{
	None UMETA(DisplayName = "None"),
	EnergyCable UMETA(DisplayName = "Energy Cable"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	EnergyStorage UMETA(DisplayName = "Energy Storage"),
	MiningMachine UMETA(DisplayName = "Mining Machine"),
	WaterCollector UMETA(DisplayName = "Water Collector"),
	ConcretePlant UMETA(DisplayName = "Concrete Plant"),
	CommunicationModule UMETA(DisplayName = "Communication Module"),
	CargoBay UMETA(DisplayName = "Cargo Bay"),
	CommandHub UMETA(DisplayName = "Command Hub"),
	SolarArray UMETA(DisplayName = "Solar Array"),
	WindGenerator UMETA(DisplayName = "Wind Generator"),
	GeothermalPlant UMETA(DisplayName = "Geothermal Plant"),
	NuclearReactor UMETA(DisplayName = "Nuclear Reactor"),
	MiningStation UMETA(DisplayName = "Mining Station"),
	ResourceStorage UMETA(DisplayName = "Resource Storage"),
	DroneFactory UMETA(DisplayName = "Drone Factory"),
	CommunicationsTower UMETA(DisplayName = "Communications Tower"),
	Steelworks UMETA(DisplayName = "Steelworks")
};

UENUM(BlueprintType)
enum class ESTPBuildCategory : uint8
{
	Energy UMETA(DisplayName = "Energy Build"),
	Industry UMETA(DisplayName = "Industry"),
	Logistics UMETA(DisplayName = "Logistics"),
	Infrastructure UMETA(DisplayName = "Infrastructure")
};
