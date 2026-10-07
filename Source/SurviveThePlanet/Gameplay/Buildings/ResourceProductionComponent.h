#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "ResourceProductionComponent.generated.h"

/** Advances atomic material-conversion cycles for production buildings. */
UCLASS()
class SURVIVETHEPLANET_API UResourceProductionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UResourceProductionComponent();

	bool UpdateProduction(float DeltaSeconds, bool bCanRun, const TArray<FResourceCost>& Inputs,
		EResourceType OutputResource, int32 OutputPerCycle, float CycleSeconds);
	void StopProduction() { bIsProducing = false; }

private:
	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	float CycleProgressSeconds = 0.0f;
	bool bIsProducing = false;
};
