#include "GeothermalSite.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Resources/GeothermalEffectsComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AGeothermalSite::AGeothermalSite()
{
 SiteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SiteMesh"));
 SetRootComponent(SiteMesh);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Game/Geothermal/SM_GeothermalSite.SM_GeothermalSite"));
 if (Mesh.Succeeded()) SiteMesh->SetStaticMesh(Mesh.Object);
 SiteMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 SiteMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
 SiteMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 Effects = CreateDefaultSubobject<UGeothermalEffectsComponent>(TEXT("GeothermalEffects"));
}
ABaseBuilding* AGeothermalSite::GetOccupant() const { return Occupant.Get(); }
bool AGeothermalSite::IsAvailable() const
{
 return !Occupant.IsValid() || Occupant->IsActorBeingDestroyed();
}
bool AGeothermalSite::Claim(ABaseBuilding* Building)
{
 if (!IsValid(Building) || !IsAvailable()) return false;
 Occupant = Building;
 return true;
}
AGeothermalSite* AGeothermalSite::FindAt(const UWorld* World, const FVector& Location, float MaxDistance)
{
 if (!World) return nullptr;
 AGeothermalSite* Closest = nullptr;
 double BestDistance = FMath::Square(MaxDistance);
 for (TActorIterator<AGeothermalSite> It(World); It; ++It)
 {
  if (It->IsActorBeingDestroyed() || It->IsHidden()) continue;
  const double Distance = FVector::DistSquared2D(Location, It->GetActorLocation());
  if (Distance <= BestDistance) { BestDistance = Distance; Closest = *It; }
 }
 return Closest;
}
