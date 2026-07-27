#pragma once

#include "CoreMinimal.h"
#include "BuildToolTypes.generated.h"

UENUM(BlueprintType)
enum class ESTPBuildTool : uint8
{
	None UMETA(DisplayName = "None"),
	EnergyCable UMETA(DisplayName = "Energy Cable"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	MiningMachine UMETA(DisplayName = "Mining Machine")
};
