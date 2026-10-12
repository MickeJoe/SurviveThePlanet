#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IndustrialEffectsComponent.generated.h"
class UPointLightComponent;
class UNiagaraComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class APlanetWeatherManager;

UENUM(BlueprintType)
enum class EIndustrialEffectKind : uint8 { UraniumDeposit, NuclearReactor, UraniumProcessor, UraniumMine, WaterTreatment, CoolingPlant, SurfaceMine };

/** Pooled wind-driven particles and operational indicators for colony industry. */
UCLASS(ClassGroup=(Effects), meta=(BlueprintSpawnableComponent))
class SURVIVETHEPLANET_API UIndustrialEffectsComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UIndustrialEffectsComponent();
 virtual void BeginPlay() override;
 virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects") EIndustrialEffectKind EffectKind = EIndustrialEffectKind::NuclearReactor;
private:
 UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> Emitters;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Indicators;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Fan;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MineDrill;
 UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> WorkLights;
 FVector DrillRestLocation = FVector::ZeroVector;
 TWeakObjectPtr<APlanetWeatherManager> WeatherManager;
 float Elapsed = 0.0f;
 bool bEffectsActive = false;
};
