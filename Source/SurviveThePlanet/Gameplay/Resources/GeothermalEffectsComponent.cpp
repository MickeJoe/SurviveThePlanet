#include "GeothermalEffectsComponent.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Resources/GeothermalSite.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"

UGeothermalEffectsComponent::UGeothermalEffectsComponent()
{
 PrimaryComponentTick.bCanEverTick = true;
 PrimaryComponentTick.TickInterval = 0.1f;
}

void UGeothermalEffectsComponent::BeginPlay()
{
 Super::BeginPlay();
 AActor* Owner = GetOwner();
 if (!Owner) return;

 const ABaseBuilding* Building = Cast<ABaseBuilding>(Owner);
 if (Building && Building->IsPlacementPreview())
 {
  SetComponentTickEnabled(false);
  return;
 }

 USceneComponent* Anchor = Building ? Owner->FindComponentByClass<UStaticMeshComponent>() : Owner->GetRootComponent();
 if (!Anchor) return;

 for (TActorIterator<APlanetWeatherManager> It(GetWorld()); It; ++It)
 {
  WeatherManager = *It;
  break;
 }

 UNiagaraSystem* SteamSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Geothermal/NS_GeothermalSteam.NS_GeothermalSteam"));
 if (SteamSystem)
 {
  for (const FVector& Vent : VentLocations)
  {
   UNiagaraComponent* Steam = NewObject<UNiagaraComponent>(Owner);
   Steam->SetupAttachment(Anchor);
   Steam->SetRelativeLocation(Vent);
   // Uncovered fissures vent less vapor than an operating heat exchanger.
   Steam->SetRelativeScale3D(FVector(Building ? 1.0f : 0.7f));
   Steam->SetAutoActivate(false);
   Steam->SetAsset(SteamSystem);
   Steam->SetCastShadow(false);
   Owner->AddInstanceComponent(Steam);
   Steam->RegisterComponent();
   SteamEmitters.Add(Steam);
  }
 }

 HeatLight = NewObject<UPointLightComponent>(Owner);
 HeatLight->SetupAttachment(Anchor);
 HeatLight->SetRelativeLocation(FVector(0,0,30));
 HeatLight->SetLightColor(FLinearColor(1.0f,0.15f,0.015f));
 HeatLight->SetAttenuationRadius(230);
 HeatLight->SetCastShadows(false);
 Owner->AddInstanceComponent(HeatLight);
 HeatLight->RegisterComponent();

 if (Building)
 {
  for (const FVector& Position : {FVector(-122,-106,48), FVector(122,-106,48), FVector(-122,106,48), FVector(122,106,48)})
  {
   UPointLightComponent* Lamp = NewObject<UPointLightComponent>(Owner);
   Lamp->SetupAttachment(Anchor);
   Lamp->SetRelativeLocation(Position);
   Lamp->SetLightColor(FLinearColor(1.0f,0.72f,0.38f));
   Lamp->SetAttenuationRadius(135);
   Lamp->SetCastShadows(false);
   Owner->AddInstanceComponent(Lamp);
   Lamp->RegisterComponent();
   WorkLights.Add(Lamp);
  }
 }
}

void UGeothermalEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
 Super::TickComponent(DeltaTime, TickType, TickFunction);
 Elapsed += DeltaTime;

 const AActor* Owner = GetOwner();
 if (!Owner) return;
 const ABaseBuilding* Building = Cast<ABaseBuilding>(Owner);
 const AGeothermalSite* Site = Cast<AGeothermalSite>(Owner);
 const bool bVisible = !Owner->IsHidden();
 const bool bOperational = !Building || (!Building->IsPlacementPreview() && Building->IsOperational());
 // A claimed site's steam is routed through the plant's two exhausts.
 const bool bEmitSteam = bVisible && bOperational && (!Site || Site->IsAvailable());

 FPlanetWeatherState Weather;
 Weather.WindPercent = 4.0f;
 if (WeatherManager.IsValid()) Weather = WeatherManager->GetCurrentWeather();
 const FVector Wind(FMath::Clamp(Weather.WindPercent,0.0f,25.0f) * 2.5f, 4.0f, 0.0f);
 for (UNiagaraComponent* Steam : SteamEmitters)
 {
  Steam->SetVariableVec3(TEXT("User.Wind"), Wind);
  const float Brightness = 0.22f + 0.48f * FMath::Clamp(Weather.SunPercent / 100.0f,0.0f,1.0f);
  Steam->SetVariableLinearColor(TEXT("User.SteamColor"), FLinearColor(Brightness, Brightness * 1.04f, Brightness * 1.08f, Building ? 0.58f : 0.32f));
  Steam->SetVisibility(bEmitSteam);
  if (bEmitSteam != bSteamActive)
  {
   if (bEmitSteam) Steam->Activate(true);
   else Steam->DeactivateImmediate();
  }
 }
 bSteamActive = bEmitSteam;

 if (HeatLight) HeatLight->SetIntensity(bVisible ? 115.0f + 18.0f * FMath::Sin(Elapsed * 1.3f) : 0.0f);
 const float Darkness = 1.0f - FMath::Clamp(Weather.SunPercent / 35.0f,0.0f,1.0f);
 for (UPointLightComponent* Lamp : WorkLights)
 {
  Lamp->SetIntensity(bVisible && bOperational ? 30.0f * Darkness : 0.0f);
 }
}
