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
	CargoBay UMETA(DisplayName = "Cargo Bay")
};
