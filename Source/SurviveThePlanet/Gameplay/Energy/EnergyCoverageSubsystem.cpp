#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UEnergyCoverageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
 Super::OnWorldBeginPlay(InWorld);
 if (!InWorld.IsGameWorld()) return;

 // Coverage and consumers need the energy budget even in maps without a placed manager.
 for (TActorIterator<ACableNetworkManager> It(&InWorld); It; ++It)
 {
  if (IsValid(*It) && !It->IsActorBeingDestroyed()) return;
 }
 InWorld.SpawnActor<ACableNetworkManager>();
}

void UEnergyCoverageSubsystem::RegisterSource(UEnergyCoverageComponent* Source)
{
	if (!IsValid(Source)) return;
	Sources.AddUnique(Source);
	++SourceRevision;
	OnSourcesChanged.Broadcast();
	RefreshPoweredCoverageVisualization();
}

void UEnergyCoverageSubsystem::UnregisterSource(UEnergyCoverageComponent* Source)
{
	Sources.Remove(Source);
	++SourceRevision;
	OnSourcesChanged.Broadcast();
	RefreshPoweredCoverageVisualization();
}

void UEnergyCoverageSubsystem::SetCoverageVisualizationVisible(bool bVisible)
{
	bVisualizationVisible = bVisible;
	Sources.RemoveAll([](const TWeakObjectPtr<UEnergyCoverageComponent>& Source) { return !Source.IsValid(); });
	for (const TWeakObjectPtr<UEnergyCoverageComponent>& Source : Sources)
	{
		Source->SetCoverageVisualizationVisible(bVisible);
	}
}

void UEnergyCoverageSubsystem::RefreshSource(UEnergyCoverageComponent* Source)
{
	if (IsValid(Source) && Sources.Contains(Source))
	{
		RefreshPoweredCoverageVisualization();
		++SourceRevision;
		OnSourcesChanged.Broadcast();
	}
}

UEnergyCoverageComponent* UEnergyCoverageSubsystem::FindNearestSource(
 const FVector& Location, const AActor* ExcludedOwner,
 UEnergyCoverageComponent* CurrentSource, float SwitchMargin) const
{
 auto CanConnect = [&](UEnergyCoverageComponent* Candidate)
 {
  if (!Candidate || Candidate->GetOwner() == ExcludedOwner || !Candidate->IsEligibleSource())
   return false;
  // When a parent is removed, reconnect only to sources outside our own branch.
  const AActor* Ancestor = Candidate->GetOwner();
  for (int32 Depth = 0; Ancestor && Depth < 64; ++Depth)
  {
   if (Ancestor == ExcludedOwner) return false;
   const auto* Connection = Ancestor->FindComponentByClass<UEnergyConnectionComponent>();
   if (!Connection) return true;
   Ancestor = Connection->GetSourceActor();
  }
  return Ancestor == nullptr;
 };
 UEnergyCoverageComponent* Nearest = nullptr;
 float BestDistance = TNumericLimits<float>::Max();
 for (const auto& Entry : Sources)
 {
  UEnergyCoverageComponent* Candidate = Entry.Get();
  if (!CanConnect(Candidate)) continue;
  const float Distance = FVector::Dist2D(Location, Candidate->GetComponentLocation());
  if (Distance < BestDistance)
  {
   BestDistance = Distance;
   Nearest = Candidate;
  }
 }
 if (CurrentSource && Sources.Contains(CurrentSource) && CanConnect(CurrentSource)
  && FVector::Dist2D(Location, CurrentSource->GetComponentLocation())
   <= BestDistance + FMath::Max(0.0f, SwitchMargin))
  return CurrentSource;
 return Nearest;
}


bool UEnergyCoverageSubsystem::IsSourceConnectedToPowerGrid(const UEnergyCoverageComponent* Source) const
{
 TSet<const AActor*> Visited;
 const UEnergyCoverageComponent* Current = Source;
 while (Current && Current->IsEligibleSource() && Sources.Contains(const_cast<UEnergyCoverageComponent*>(Current)))
 {
  const AActor* Owner = Current->GetOwner();
  if (Visited.Contains(Owner)) return false;
  Visited.Add(Owner);
  const ABaseBuilding* Building = Cast<ABaseBuilding>(Owner);
  if (!Building || Building->GetBuildingType() == ESTPBuildingType::BaseModule) return true;
  const UEnergyConnectionComponent* Connection = Owner->FindComponentByClass<UEnergyConnectionComponent>();
  if (!Connection || !Connection->HasUsableConnection()) return false;
  Current = Connection->GetSourceCoverage();
 }
 return false;
}

bool UEnergyCoverageSubsystem::IsLocationConnectedToPowerGrid(const FVector& Location) const
{
 for (const auto& Entry : Sources)
 {
  const UEnergyCoverageComponent* Source = Entry.Get();
  if (!Source || !Source->IsEligibleSource()) continue;
  const double Radius = Source->CoverageRadius;
  if (FVector::DistSquared2D(Location, Source->GetComponentLocation()) <= Radius * Radius
   && IsSourceConnectedToPowerGrid(Source)) return true;
 }
 return false;
}

void UEnergyCoverageSubsystem::RefreshPoweredCoverageVisualization()
{
 for (const auto& Entry : Sources)
  if (UEnergyCoverageComponent* Source = Entry.Get())
   Source->SetCoverageVisualizationVisible(bVisualizationVisible);
}
