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
	UFUNCTION(BlueprintPure, Category = "Trading|Dock")
	FTransform GetMerchantDockTransform() const;
	/** Docking point on the loading deck, in mesh-local centimetres. */
	UPROPERTY(EditAnywhere, Category = "Trading|Dock")
	FVector MerchantDockOffset = FVector(0.0f, 130.0f, -114.0f);
	UPROPERTY(EditAnywhere, Category = "Trading|Dock")
	FRotator MerchantDockRotation = FRotator::ZeroRotator;
};
