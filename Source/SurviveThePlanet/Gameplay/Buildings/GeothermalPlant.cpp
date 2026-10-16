#include "GeothermalPlant.h"
#include "Gameplay/Resources/GeothermalSite.h"
#include "Gameplay/Resources/GeothermalEffectsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGeothermalPlant::AGeothermalPlant()
{
 bGroundMeshToSurface = true;
 BuildingTag = TEXT("GeothermalPlant");
 BuildingType = ESTPBuildingType::Other;
 Effects = CreateDefaultSubobject<UGeothermalEffectsComponent>(TEXT("GeothermalEffects"));
 // FBX import preserves X/Z and mirrors Blender Y.
 Effects->VentLocations = {FVector(-65,-30,210),FVector(65,-90,163)};
 TurbineRotor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurbineRotor"));
 TurbineRotor->SetupAttachment(BuildingMesh);
 TurbineRotor->SetRelativeLocation(FVector(74,65,57));
 TurbineRotor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Rotor(TEXT("/Game/Geothermal/SM_GeothermalRotor.SM_GeothermalRotor"));
 if (Rotor.Succeeded()) TurbineRotor->SetStaticMesh(Rotor.Object);
}
void AGeothermalPlant::BeginPlay()
{
 Super::BeginPlay();
 if (!IsPlacementPreview())
 {
  if (AGeothermalSite* Site = AGeothermalSite::FindAt(GetWorld(), GetActorLocation(), 40)) Site->Claim(this);
 }
}
void AGeothermalPlant::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 TurbineRotor->SetVisibility(!IsPlacementPreview() && GetConstructionProgress() >= 1.0f);
 if (CanProduce()) TurbineRotor->AddLocalRotation(FRotator(0,0,DeltaSeconds * 180.0f));
}
