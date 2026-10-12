#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "NuclearReactor.generated.h"
class UResourceProductionComponent;
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ANuclearReactor : public ABaseBuilding
{
 GENERATED_BODY()
public:
 ANuclearReactor();
 virtual void Tick(float DeltaSeconds) override;
 virtual float GetEnergyProductionPerMinute() const override;
 UFUNCTION(BlueprintPure, Category="Reactor")
 bool IsProducing() const { return bIsProducing; }
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reactor", meta=(ClampMin="1"))
 int32 FuelRodsPerCycle = 1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reactor", meta=(ClampMin="1"))
 int32 CoolingUnitsPerCycle = 1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reactor", meta=(ClampMin="0.1"))
 float CycleSeconds = 60.0f;
private:
 UPROPERTY(VisibleAnywhere)
 TObjectPtr<UResourceProductionComponent> Production;
 bool bIsProducing = false;
};
