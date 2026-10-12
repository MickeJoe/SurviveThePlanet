#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeothermalSite.generated.h"
class ABaseBuilding;
class UStaticMeshComponent;
class UGeothermalEffectsComponent;

/** A permanent heat source. Construction occupies it without consuming the resource. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AGeothermalSite : public AActor
{
 GENERATED_BODY()
public:
 AGeothermalSite();
 UFUNCTION(BlueprintPure, Category="Geothermal") bool IsAvailable() const;
 UFUNCTION(BlueprintCallable, Category="Geothermal") bool Claim(ABaseBuilding* Building);
 UFUNCTION(BlueprintPure, Category="Geothermal") ABaseBuilding* GetOccupant() const;
 static AGeothermalSite* FindAt(const UWorld* World, const FVector& Location, float MaxDistance = 160.0f);
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> SiteMesh;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UGeothermalEffectsComponent> Effects;
private:
 TWeakObjectPtr<ABaseBuilding> Occupant;
};
