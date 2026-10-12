#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "UraniumProcessor.generated.h"

class UResourceProductionComponent;

/** Refines uranium using the shared atomic recipe and drone staffing systems. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AUraniumProcessor : public ABaseBuilding
{
 GENERATED_BODY()
public:
 AUraniumProcessor();
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(BlueprintPure, Category="Uranium Production")
 bool IsProducing() const { return bIsProducing; }
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Uranium Production", meta=(ClampMin="1"))
 int32 OrePerCycle = 2;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Uranium Production", meta=(ClampMin="1"))
 int32 FuelRodsPerCycle = 1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Uranium Production", meta=(ClampMin="0.1"))
 float CycleSeconds = 20.0f;
private:
 UPROPERTY(VisibleAnywhere)
 TObjectPtr<UResourceProductionComponent> Production;
 bool bIsProducing = false;
};
