#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "GeothermalPlant.generated.h"
class UGeothermalEffectsComponent;
class UStaticMeshComponent;
class AGeothermalSite;

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AGeothermalPlant : public ABaseBuilding
{
 GENERATED_BODY()
public:
 AGeothermalPlant();
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UGeothermalEffectsComponent> Effects;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> TurbineRotor;
protected:
 virtual void BeginPlay() override;
};
