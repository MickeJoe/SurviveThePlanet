#include "Gameplay/Exploration/FogOfWarSubsystem.h"

#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Resources/BaseResourceSource.h"

void UFogOfWarSubsystem::Initialize(FSubsystemCollectionBase& Collection) { Super::Initialize(Collection); RefreshVisibility(); }
void UFogOfWarSubsystem::Deinitialize() { RevealSources.Reset(); DiscoveredResources.Reset(); Super::Deinitialize(); }

void UFogOfWarSubsystem::Tick(float DeltaTime)
{
	RefreshAccumulator += DeltaTime;
	if (RefreshAccumulator >= 0.25f) { RefreshAccumulator = 0.0f; RefreshVisibility(); }
}

TStatId UFogOfWarSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UFogOfWarSubsystem, STATGROUP_Tickables); }
bool UFogOfWarSubsystem::IsTickable() const { return GetWorld() && GetWorld()->IsGameWorld(); }

bool UFogOfWarSubsystem::IsWorldLocationVisible(FVector WorldLocation) const
{
	for (const ABaseBuilding* Source : RevealSources)
	{
		if (!IsValid(Source) || !Source->ProvidesVision()) continue;
		const FVector Delta = WorldLocation - Source->GetActorLocation();
		if (FVector2D(Delta.X, Delta.Y).SquaredLength() <= FMath::Square(Source->GetVisionRadius())) return true;
	}
	return false;
}

bool UFogOfWarSubsystem::IsResourceDiscovered(const ABaseResourceSource* Resource) const
{
	return IsValid(Resource) && DiscoveredResources.Contains(Resource);
}

TArray<ABaseResourceSource*> UFogOfWarSubsystem::GetDiscoveredResources() const
{
	TArray<ABaseResourceSource*> Result;
	for (ABaseResourceSource* Resource : DiscoveredResources) if (IsValid(Resource)) Result.Add(Resource);
	return Result;
}

void UFogOfWarSubsystem::RefreshVisibility() { RefreshRevealSources(); RefreshResources(); }

void UFogOfWarSubsystem::RefreshRevealSources()
{
	RevealSources.Reset();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABaseBuilding> It(World); It; ++It) if (It->ProvidesVision()) RevealSources.Add(*It);
	}
}

void UFogOfWarSubsystem::RefreshResources()
{
	UWorld* World = GetWorld();
	if (!World) return;
	for (TActorIterator<ABaseResourceSource> It(World); It; ++It)
	{
		ABaseResourceSource* Resource = *It;
		const bool bVisibleNow = IsWorldLocationVisible(Resource->GetActorLocation());
		const bool bWasDiscovered = DiscoveredResources.Contains(Resource);
		Resource->SetFogOfWarVisible(bVisibleNow || (bWasDiscovered && Resource->RemainsVisibleAfterDiscovery()));
		if (bVisibleNow && !bWasDiscovered)
		{
			DiscoveredResources.Add(Resource);
			OnResourceDiscovered.Broadcast(Resource);
		}
	}
}
