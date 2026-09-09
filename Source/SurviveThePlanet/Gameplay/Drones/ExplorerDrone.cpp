#include "Gameplay/Drones/ExplorerDrone.h"

#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "UObject/ConstructorHelpers.h"

AExplorerDrone::AExplorerDrone()
{
	PrimaryActorTick.bCanEverTick = true;
	DroneDisplayName = NSLOCTEXT("SurviveThePlanet", "ExplorerDroneName", "Explorer Drone");
	bEnableHoverAnimation = true;
	MoveSpeed = ExplorationFlightSpeed;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ExplorerMesh(TEXT("/Game/Models/Units/ExplorerDrone/SM_ExplorerDrone.SM_ExplorerDrone"));
	if (ExplorerMesh.Succeeded())
	{
		DroneMesh->SetStaticMesh(ExplorerMesh.Object);
	}
}

bool AExplorerDrone::CanActivateExploration() const
{
	if (bIsExploring)
	{
		return false;
	}
	AHexSectorGrid* Grid = SectorGrid ? SectorGrid.Get() : FindSectorGrid();
	return Grid && !Grid->GetNearestUndiscoveredSectorIds(GetActorLocation(), 1).IsEmpty();
}

TArray<int32> AExplorerDrone::GetPreviewSectorIds() const
{
	AHexSectorGrid* Grid = SectorGrid ? SectorGrid.Get() : FindSectorGrid();
	return Grid ? Grid->GetNearestUndiscoveredSectorIds(GetActorLocation(), SectorsPerMission) : TArray<int32>();
}

void AExplorerDrone::SetActivationPreviewVisible(bool bVisible)
{
	bShowActivationPreview = bVisible && !bIsExploring;
	SetActorTickEnabled(bShowActivationPreview || bIsExploring);
}

bool AExplorerDrone::ActivateExploration()
{
	if (!CanActivateExploration())
	{
		return false;
	}
	SectorGrid = FindSectorGrid();
	RefreshTargets();
	if (!SectorGrid || TargetSectorIds.IsEmpty())
	{
		return false;
	}
	CurrentTargetIndex = 0;
	ScannedSectorCount = 0;
	bIsExploring = true;
	bShowActivationPreview = false;
	SetActorTickEnabled(true);
	OnExplorationProgress.Broadcast(0, TargetSectorIds.Num());
	return true;
}

void AExplorerDrone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bShowActivationPreview && !bIsExploring)
	{
		DrawActivationPreview();
	}
	if (!bIsExploring || !SectorGrid || !TargetSectorIds.IsValidIndex(CurrentTargetIndex))
	{
		return;
	}

	FHexSector TargetSector;
	if (!SectorGrid->GetSectorById(TargetSectorIds[CurrentTargetIndex], TargetSector))
	{
		FinishExploration();
		return;
	}
	const FVector TargetLocation = TargetSector.WorldCenter + FVector(0.0f, 0.0f, FlightHeight);
	const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, DeltaSeconds, ExplorationFlightSpeed);
	SetActorLocation(NewLocation);
	const FVector Direction = TargetLocation - NewLocation;
	if (!Direction.IsNearlyZero()) SetActorRotation(Direction.Rotation());

	if (FVector::DistSquared(NewLocation, TargetLocation) <= FMath::Square(ArrivalRadius))
	{
		SectorGrid->SetSectorState(TargetSector.Id, ESectorState::Discovered);
		++ScannedSectorCount;
		++CurrentTargetIndex;
		OnExplorationProgress.Broadcast(ScannedSectorCount, TargetSectorIds.Num());
		if (!TargetSectorIds.IsValidIndex(CurrentTargetIndex)) FinishExploration();
	}
}

void AExplorerDrone::RefreshTargets()
{
	TargetSectorIds = SectorGrid ? SectorGrid->GetNearestUndiscoveredSectorIds(GetActorLocation(), SectorsPerMission) : TArray<int32>();
}

void AExplorerDrone::DrawActivationPreview() const
{
	AHexSectorGrid* Grid = SectorGrid ? SectorGrid.Get() : FindSectorGrid();
	if (!Grid || !GetWorld()) return;
	const TArray<int32> PreviewIds = Grid->GetNearestUndiscoveredSectorIds(GetActorLocation(), SectorsPerMission);
	const FColor Color = PreviewColor.ToFColor(true);
	FVector Previous = GetActorLocation();
	for (int32 Index = 0; Index < PreviewIds.Num(); ++Index)
	{
		FHexSector Sector;
		if (!Grid->GetSectorById(PreviewIds[Index], Sector)) continue;
		const FVector Marker = Sector.WorldCenter + FVector(0.0f, 0.0f, 120.0f);
		DrawDebugLine(GetWorld(), Previous, Marker, Color, false, 0.0f, 0, 8.0f);
		DrawDebugCircle(GetWorld(), Marker, 180.0f, 32, Color, false, 0.0f, 0, 10.0f, FVector::ForwardVector, FVector::RightVector, false);
		DrawDebugString(GetWorld(), Marker + FVector(0, 0, 90), FString::FromInt(Index + 1), nullptr, Color, 0.0f, true, 1.6f);
		Previous = Marker;
	}
}

void AExplorerDrone::FinishExploration()
{
	bIsExploring = false;
	bShowActivationPreview = false;
	SetActorTickEnabled(false);
	OnExplorationCompleted.Broadcast();
	Destroy();
}

AHexSectorGrid* AExplorerDrone::FindSectorGrid() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AHexSectorGrid> It(World); It; ++It) return *It;
	}
	return nullptr;
}
