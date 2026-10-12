#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GeothermalEffectsComponent.generated.h"

class UNiagaraComponent;
class UPointLightComponent;
class APlanetWeatherManager;

/** Shared heat glow and wind-driven vapor for geothermal sites and plants. */
UCLASS(ClassGroup=(Effects), meta=(BlueprintSpawnableComponent))
class SURVIVETHEPLANET_API UGeothermalEffectsComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UGeothermalEffectsComponent();
 virtual void BeginPlay() override;
 virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

 UPROPERTY(EditAnywhere, Category="Geothermal")
 TArray<FVector> VentLocations = {FVector(-90,40,18), FVector(85,-40,18)};

private:
 UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> SteamEmitters;
 UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> WorkLights;
 UPROPERTY(Transient) TObjectPtr<UPointLightComponent> HeatLight;
 TWeakObjectPtr<APlanetWeatherManager> WeatherManager;
 float Elapsed = 0.0f;
 bool bSteamActive = false;
};
