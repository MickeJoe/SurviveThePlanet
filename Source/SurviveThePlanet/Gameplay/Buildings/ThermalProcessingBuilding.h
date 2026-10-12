#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "ThermalProcessingBuilding.generated.h"

class UResourceProductionComponent;
class UStaticMeshComponent;
class UIndustrialEffectsComponent;

UENUM(BlueprintType)
enum class EThermalRecipe : uint8 { WaterPurification, CoolingUnits };

/** Water treatment and coolant manufacture share the atomic production system. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AThermalProcessingBuilding : public ABaseBuilding
{
 GENERATED_BODY()
public:
 AThermalProcessingBuilding();
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(BlueprintPure, Category="Production") bool IsProducing() const { return bIsProducing; }
 TArray<FResourceCost> GetRecipeInputs() const;
 EResourceType GetRecipeOutput() const;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production") EThermalRecipe Recipe = EThermalRecipe::WaterPurification;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production", meta=(ClampMin="1")) int32 PrimaryInputPerCycle = 2;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production", meta=(ClampMin="1")) int32 PurifiedWaterPerCycle = 1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production", meta=(ClampMin="1")) int32 OutputPerCycle = 1;
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production", meta=(ClampMin="0.1")) float CycleSeconds = 20.0f;
private:
 UPROPERTY(VisibleAnywhere) TObjectPtr<UResourceProductionComponent> Production;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MotionFan;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UIndustrialEffectsComponent> IndustrialEffects;
 bool bIsProducing = false;
};
