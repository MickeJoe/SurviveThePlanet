#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "CargoBay.generated.h"

/** A placeable cargo storage and logistics building. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ACargoBay : public AEnergyModule
{
	GENERATED_BODY()

public:
	ACargoBay();
};
