#include "IndustrialEffectsComponent.h"
#include "ThermalProcessingBuilding.h"
#include "CrudeWorkshop.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Buildings/UraniumProcessor.h"
#include "Gameplay/Buildings/NuclearReactor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "EngineUtils.h"

UIndustrialEffectsComponent::UIndustrialEffectsComponent()
{
 PrimaryComponentTick.bCanEverTick = true;
 PrimaryComponentTick.TickInterval = 0.05f;
}

void UIndustrialEffectsComponent::BeginPlay()
{
 Super::BeginPlay();
 AActor* Owner = GetOwner();
 const ABaseBuilding* Building = Cast<ABaseBuilding>(Owner);
 if (!Owner || (Building && Building->IsPlacementPreview())) { SetComponentTickEnabled(false); return; }
 const bool bDeposit = EffectKind == EIndustrialEffectKind::UraniumDeposit;
 const bool bMine = EffectKind == EIndustrialEffectKind::UraniumMine || EffectKind == EIndustrialEffectKind::SurfaceMine;
 const bool bDust = bDeposit || bMine;
 USceneComponent* Anchor = Owner->GetRootComponent();
 TArray<UStaticMeshComponent*> Meshes;
 Owner->GetComponents(Meshes);
 for (UStaticMeshComponent* Mesh : Meshes)
 {
  if (Mesh->GetFName() == TEXT("BuildingMesh")) Anchor = Mesh;
  if (Mesh->GetName().StartsWith(TEXT("Motion")) && Mesh->GetStaticMesh()) Fan = Mesh;
  if (Mesh->GetName().StartsWith(TEXT("MineDrill")))
  {
   MineDrill = Mesh;
   DrillRestLocation = Mesh->GetRelativeLocation();
  }
  for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
  {
   UMaterialInterface* Material = Mesh->GetMaterial(Slot);
   if (Material && (Material->GetName().Contains(TEXT("UraniumMineral")) || Material->GetName().Contains(TEXT("NuclearStatus"))))
    if (UMaterialInstanceDynamic* Indicator = Mesh->CreateDynamicMaterialInstance(Slot)) Indicators.Add(Indicator);
  }
 }
 for (TActorIterator<APlanetWeatherManager> It(GetWorld()); It; ++It) { WeatherManager = *It; break; }
 const TCHAR* SystemPath = bDust ? TEXT("/Game/Uranium/NS_UraniumDust.NS_UraniumDust")
  : EffectKind == EIndustrialEffectKind::NuclearReactor ? TEXT("/Game/Uranium/NS_NuclearSteam.NS_NuclearSteam")
  : TEXT("/Game/Geothermal/NS_GeothermalSteam.NS_GeothermalSteam");
 UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, SystemPath);
 if (!System || !Anchor) return;
 TArray<FVector> Vents;
 if (EffectKind == EIndustrialEffectKind::CrudeWorkshop) Vents = {FVector(87,-75,260)};
 else if (bMine) Vents = {FVector(-80,0,48)};
 else if (bDeposit) Vents = {FVector(-35,15,18)};
 else if (EffectKind == EIndustrialEffectKind::NuclearReactor) Vents = {FVector(137,105,208),FVector(137,-88,208)};
 else if (EffectKind == EIndustrialEffectKind::WaterTreatment) Vents = {FVector(-125,-70,246)};
 else if (EffectKind == EIndustrialEffectKind::CoolingPlant) Vents = {FVector(0,-30,214)};
 else Vents = {FVector(-78,-9,250),FVector(78,-9,250)};
 for (const FVector& Vent : Vents)
 {
  UNiagaraComponent* Particles = NewObject<UNiagaraComponent>(Owner);
  Particles->SetupAttachment(Anchor);
  Particles->SetRelativeLocation(Vent);
  Particles->SetRelativeScale3D(FVector(EffectKind == EIndustrialEffectKind::CrudeWorkshop ? 0.12f : bDust ? 1.0f : EffectKind == EIndustrialEffectKind::NuclearReactor ? 0.42f : 0.26f));
  Particles->SetAutoActivate(false); Particles->SetAsset(System); Particles->SetCastShadow(false);
  Owner->AddInstanceComponent(Particles); Particles->RegisterComponent(); Emitters.Add(Particles);
 }
 if (bMine || EffectKind == EIndustrialEffectKind::CrudeWorkshop)
 {
  const TArray<FVector> LightPositions = EffectKind == EIndustrialEffectKind::CrudeWorkshop
   ? TArray<FVector>{FVector(-62,-16,133)}
   : TArray<FVector>{FVector(-178,-155,40), FVector(178,-155,40), FVector(-178,155,40), FVector(178,155,40)};
  for (const FVector& Position : LightPositions)
  {
   UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
   if (EffectKind == EIndustrialEffectKind::CrudeWorkshop) Light->ComponentTags.Add(TEXT("ProductionEffect"));
   Light->SetupAttachment(Anchor);
   Light->SetRelativeLocation(Position);
   Light->SetLightColor(FLinearColor(1.0f, 0.78f, 0.48f));
   Light->SetAttenuationRadius(160.0f);
   Light->SetCastShadows(false);
   Owner->AddInstanceComponent(Light);
   Light->RegisterComponent();
   WorkLights.Add(Light);
  }
 }
}

void UIndustrialEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
 Super::TickComponent(DeltaTime, TickType, TickFunction);
 Elapsed += DeltaTime;
 const AActor* Owner = GetOwner();
 if (!Owner) return;
 const ABaseBuilding* Building = Cast<ABaseBuilding>(Owner);
 const ABaseResourceSource* Deposit = Cast<ABaseResourceSource>(Owner);
 const AMiningMachine* Mine = (EffectKind == EIndustrialEffectKind::UraniumMine || EffectKind == EIndustrialEffectKind::SurfaceMine) ? Cast<AMiningMachine>(Owner) : nullptr;
 const ANuclearReactor* Reactor = Cast<ANuclearReactor>(Owner);
 const AUraniumProcessor* Processor = Cast<AUraniumProcessor>(Owner);
 const ACrudeWorkshop* Workshop = Cast<ACrudeWorkshop>(Owner);
 const AThermalProcessingBuilding* Thermal = Cast<AThermalProcessingBuilding>(Owner);
 const UStaticMeshComponent* DepositMesh = Deposit ? Deposit->GetResourceMeshComponent() : nullptr;
 const bool bDepositVisible = DepositMesh && DepositMesh->IsVisible() && !DepositMesh->bHiddenInGame;
 const bool bActive = !Owner->IsHidden() && !Owner->IsActorBeingDestroyed()
  && (Building ? !Building->IsPlacementPreview() && Building->CanProduce() && (!Mine || Mine->IsProducingResource()) && (!Processor || Processor->IsProducing()) && (!Reactor || Reactor->IsProducing()) && (!Thermal || Thermal->IsProducing()) && (!Workshop || Workshop->IsProducing()) : Deposit && bDepositVisible && Deposit->GetRemainingAmount() > 0);
 FPlanetWeatherState Weather;
 if (WeatherManager.IsValid()) Weather = WeatherManager->GetCurrentWeather();
 const float Brightness = 0.18f + 0.42f * FMath::Clamp(Weather.SunPercent / 100.0f, 0.0f, 1.0f);
 for (UNiagaraComponent* Particles : Emitters)
 {
  Particles->SetVariableVec3(TEXT("User.Wind"), FVector(FMath::Clamp(Weather.WindPercent,0.0f,25.0f) * 1.5f, 2, 0));
  Particles->SetVariableLinearColor(TEXT("User.SteamColor"), FLinearColor(Brightness, Brightness * 1.04f, Brightness * 1.08f, Deposit ? 0.12f : Mine ? 0.20f : 0.25f));
  Particles->SetVisibility(bActive);
  if (bActive != bEffectsActive) { if (bActive) Particles->Activate(true); else Particles->DeactivateImmediate(); }
 }
 bEffectsActive = bActive;
 const bool bLit = !Owner->IsHidden() && Building && Building->IsOperational();
 const float Darkness = 1.0f - FMath::Clamp(Weather.SunPercent / 35.0f, 0.0f, 1.0f);
 for (UPointLightComponent* Light : WorkLights)
  Light->SetIntensity(Workshop
   ? (bActive ? 55.0f + 85.0f * FMath::Pow(FMath::Max(0.0f, FMath::Sin(Elapsed * 13.0f)), 4.0f) : 0.0f)
   : (bLit ? Darkness * 45.0f : 0.0f));
 if (MineDrill && bActive)
 {
  MineDrill->AddLocalRotation(FRotator(0.0f, DeltaTime * 150.0f, 0.0f));
  MineDrill->SetRelativeLocation(DrillRestLocation + FVector(0.0f, 0.0f, FMath::Sin(Elapsed * 1.6f) * 6.0f));
 }
 // Mineral fluorescence remains faint; industrial indicators extinguish while idle.
 const float Activity = bActive ? (Deposit ? 0.15f : 0.8f) + 0.1f * FMath::Sin(Elapsed * 1.4f) : 0.0f;
 for (UMaterialInstanceDynamic* Indicator : Indicators) Indicator->SetScalarParameterValue(TEXT("Activity"), Activity);
 if (Fan && bActive && (EffectKind == EIndustrialEffectKind::UraniumProcessor || Thermal || Mine))
  Fan->AddLocalRotation(FRotator(0,DeltaTime * 160.0f,0));
}
