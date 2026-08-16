#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "CommunicationModule.generated.h"

/** A placeable communications building. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ACommunicationModule : public AEnergyModule
{
	GENERATED_BODY()

public:
	ACommunicationModule();
};
