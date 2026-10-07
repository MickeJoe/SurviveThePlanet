#include "PlanetSurfaceManager.h"
#include "TimerManager.h"
#include "SurviveThePlanet.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterVariant.h"

APlanetSurfaceManager::APlanetSurfaceManager()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void APlanetSurfaceManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateDerivedGridSize();

	if (bBuildInConstructionScript)
	{
		RebuildSurface();
	}
}

void APlanetSurfaceManager::Destroyed()
{
	ClearSurface();
	OccupiedCells.Reset();

	Super::Destroyed();
}

void APlanetSurfaceManager::RebuildSurface()
{
	const bool bHasValidChunkMesh = ChunkMeshes.ContainsByPredicate([](const TObjectPtr<UStaticMesh>& ChunkMesh)
	{
		return IsValid(ChunkMesh);
	});

	// Preserve the legacy BP_Surface1 world until the designer has assigned at
	// least one replacement chunk mesh in the Details panel.
	if (!bHasValidChunkMesh)
	{
		return;
	}

	ClearSurface();
	SpawnSurface();
}

void APlanetSurfaceManager::ClearSurface()
{
	ClearChunkComponents();

	for (AActor* Tile : SpawnedTiles)
	{
		if (IsValid(Tile))
		{
			Tile->Destroy();
		}
	}

	SpawnedTiles.Reset();
}

FSTPGridPlacement APlanetSurfaceManager::GetPlacementForWorldLocation(const FVector& WorldLocation, FIntPoint Footprint) const
{
	FSTPGridPlacement Placement;
	Footprint = SanitizeFootprint(Footprint);

	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FVector2D Offset = GetGridOffset();
	const float GridX = (LocalLocation.X + Offset.X) / TileSpacing;
	const float GridY = (LocalLocation.Y + Offset.Y) / TileSpacing;

	Placement.OriginCell = FSTPGridCell(
		FMath::RoundToInt(GridX - ((Footprint.X - 1) * 0.5f)),
		FMath::RoundToInt(GridY - ((Footprint.Y - 1) * 0.5f)));
	Placement.WorldLocation = GetWorldLocationForOriginCell(Placement.OriginCell, Footprint);
	Placement.WorldRotation = GetActorRotation();
	Placement.bValid = CanOccupyCells(Placement.OriginCell, Footprint);

	return Placement;
}

FSTPGridPlacement APlanetSurfaceManager::GetBuildingPlacementForWorldLocation(const FVector& WorldLocation, FIntPoint Footprint) const
{
	FSTPGridPlacement Placement = GetPlacementForWorldLocation(WorldLocation, Footprint);
	Placement.WorldLocation += GetActorUpVector() * BuildingPlacementHeightOffset;
	Placement.bValid = Placement.bValid && HasBuildingClearance(Placement.OriginCell, Footprint);
	return Placement;
}

int32 APlanetSurfaceManager::GetBuildingClearanceCells() const
{
	return FMath::CeilToInt(FMath::Max(200.0f, MinimumBuildingClearance) / FMath::Max(1.0f, TileSpacing));
}

bool APlanetSurfaceManager::HasBuildingClearance(FSTPGridCell OriginCell, FIntPoint Footprint, ABaseBuilding* IgnoredBuilding, bool bLogDiagnostics) const
{
	Footprint = SanitizeFootprint(Footprint);
	const bool bTerrainOverlap = !HasTerrainClearance(OriginCell, Footprint, GetBuildingClearanceCells(), bLogDiagnostics);
	if (bLogDiagnostics) UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG terrain surface=%s origin=(%d,%d) footprint=(%d,%d) OverlapsTerrainCluster=%d"), *GetName(), OriginCell.X, OriginCell.Y, Footprint.X, Footprint.Y, bTerrainOverlap);
	if (bTerrainOverlap) return false;
	const int32 Clearance = GetBuildingClearanceCells();
	const int32 CandidateMinX = OriginCell.X - Clearance;
	const int32 CandidateMinY = OriginCell.Y - Clearance;
	const int32 CandidateMaxX = OriginCell.X + Footprint.X - 1 + Clearance;
	const int32 CandidateMaxY = OriginCell.Y + Footprint.Y - 1 + Clearance;

	// Query building actors directly. Map-authored buildings are not guaranteed to
	// have been added to OccupiedCells, while placement previews must never block.
	for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
	{
		const ABaseBuilding* ExistingBuilding = *It;
		if (!IsValid(ExistingBuilding) || ExistingBuilding == IgnoredBuilding
			|| ExistingBuilding->IsActorBeingDestroyed() || ExistingBuilding->IsPlacementPreview()) continue;
		const FIntPoint ExistingFootprint = SanitizeFootprint(ExistingBuilding->GetGridFootprint());
		const FSTPGridPlacement ExistingPlacement = GetPlacementForWorldLocation(
			ExistingBuilding->GetActorLocation(), ExistingFootprint);
		const int32 ExistingMinX = ExistingPlacement.OriginCell.X;
		const int32 ExistingMinY = ExistingPlacement.OriginCell.Y;
		const int32 ExistingMaxX = ExistingMinX + ExistingFootprint.X - 1;
		const int32 ExistingMaxY = ExistingMinY + ExistingFootprint.Y - 1;
		const bool bSeparated = CandidateMaxX < ExistingMinX || CandidateMinX > ExistingMaxX
			|| CandidateMaxY < ExistingMinY || CandidateMinY > ExistingMaxY;
		if (!bSeparated)
		{
			if (bLogDiagnostics) UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG clearance_blocker actor=%s class=%s origin=(%d,%d) footprint=(%d,%d) ignored=%s preview=%d"), *ExistingBuilding->GetName(), *ExistingBuilding->GetClass()->GetPathName(), ExistingMinX, ExistingMinY, ExistingFootprint.X, ExistingFootprint.Y, *GetNameSafe(IgnoredBuilding), ExistingBuilding->IsPlacementPreview());
			return false;
		}
	}
	return true;
}

bool APlanetSurfaceManager::OverlapsTerrainCluster(FSTPGridCell OriginCell, FIntPoint Footprint, bool bLogDiagnostics) const
{
	const FTransform SurfaceTransform = GetActorTransform();
	const FVector CellCenter = SurfaceTransform.InverseTransformPosition(GetWorldLocationForCell(OriginCell));
	const FVector Min = CellCenter - FVector(TileSpacing * 0.5, TileSpacing * 0.5, 0);
	const FVector Max = Min + FVector(Footprint.X * TileSpacing, Footprint.Y * TileSpacing, 0);
	const FVector Corners[] = {
		SurfaceTransform.TransformPosition(Min),
		SurfaceTransform.TransformPosition(FVector(Max.X, Min.Y, Min.Z)),
		SurfaceTransform.TransformPosition(Max),
		SurfaceTransform.TransformPosition(FVector(Min.X, Max.Y, Min.Z))
	};
	for (TActorIterator<APlanetGeneratedSector> It(GetWorld()); It; ++It)
	{
		if (It->IsActorBeingDestroyed()) continue;
		// Test each selected mesh, not a whole-sector box. Bounds remain available
		// while ISMs are unloaded and conservatively include all transformed vertices.
		const FBox2D CandidateBounds(FVector2D(Min.X, Min.Y), FVector2D(Max.X, Max.Y));
		const FTransform SectorToSurface = It->GetActorTransform().GetRelativeTransform(SurfaceTransform);
		const FBox CombinedBounds = It->GetCombinedPlacementBounds().TransformBy(SectorToSurface);
		if (CombinedBounds.IsValid && CandidateBounds.Intersect(FBox2D(FVector2D(CombinedBounds.Min), FVector2D(CombinedBounds.Max))))
		{
			for (const FBox& MeshBounds : It->GetPlacementMeshBounds())
			{
				const FBox LocalBounds = MeshBounds.TransformBy(SectorToSurface);
				if (CandidateBounds.Intersect(FBox2D(FVector2D(LocalBounds.Min), FVector2D(LocalBounds.Max))))
				{
					if (bLogDiagnostics) UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG terrain_mesh_blocker actor=%s bounds=%s candidateMin=%s candidateMax=%s"), *It->GetName(), *LocalBounds.ToString(), *Min.ToString(), *Max.ToString());
					return true;
				}
			}
		}
		// Shell actors and their templates persist when camera residency clears meshes.
		if (!It->IsActorBeingDestroyed() && It->SectorTemplate
			&& It->SectorTemplate->OverlapsBuildingFootprint(MakeArrayView(Corners), It->GetActorTransform()))
		{
			if (bLogDiagnostics) UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG terrain_blocker actor=%s class=%s template=%s"), *It->GetName(), *It->GetClass()->GetPathName(), *It->SectorTemplate->GetPathName());
			return true;
		}
	}
	return false;
}

bool APlanetSurfaceManager::GetCellForWorldLocation(const FVector& WorldLocation, FSTPGridCell& OutCell) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FVector2D Offset = GetGridOffset();

	OutCell = FSTPGridCell(
		FMath::RoundToInt((LocalLocation.X + Offset.X) / TileSpacing),
		FMath::RoundToInt((LocalLocation.Y + Offset.Y) / TileSpacing));

	return IsCellPlayable(OutCell);
}

FVector APlanetSurfaceManager::GetWorldLocationForCell(FSTPGridCell Cell) const
{
	return GetWorldLocationForOriginCell(Cell, FIntPoint(1, 1));
}

bool APlanetSurfaceManager::IsCellInBounds(FSTPGridCell Cell) const
{
	return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < GridWidth && Cell.Y < GridHeight;
}

bool APlanetSurfaceManager::CanOccupyCells(FSTPGridCell OriginCell, FIntPoint Footprint) const
{
	Footprint = SanitizeFootprint(Footprint);

	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			if (!IsCellPlayable(Cell))
			{
				return false;
			}

			const TObjectPtr<AActor>* ExistingOccupier = OccupiedCells.Find(MakeCellKey(Cell));
			if (ExistingOccupier && IsValid(ExistingOccupier->Get()))
			{
				return false;
			}
		}
	}

	return true;
}

bool APlanetSurfaceManager::HasTerrainClearance(FSTPGridCell OriginCell, FIntPoint Footprint, int32 MarginCells, bool bLogDiagnostics) const
{
	const int32 Margin = FMath::Max(0, MarginCells);
	Footprint = SanitizeFootprint(Footprint);
	return !OverlapsTerrainCluster(FSTPGridCell(OriginCell.X - Margin, OriginCell.Y - Margin),
		Footprint + FIntPoint(2 * Margin, 2 * Margin), bLogDiagnostics);
}

bool APlanetSurfaceManager::CanReserveBuildingCells(ABaseBuilding* Building, FSTPGridCell OriginCell, FIntPoint Footprint, const AActor* ReplacedActor) const
{
	Footprint = SanitizeFootprint(Footprint);
	// Template mining checks validate terrain generation, not player build permission.
	if (Building && !Building->IsTemplate()
		&& !CanBuildInSector(Building, GetWorldLocationForOriginCell(OriginCell, Footprint))) return false;
	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			if (!IsCellPlayable(Cell)) return false;
			const TObjectPtr<AActor>* Entry = OccupiedCells.Find(MakeCellKey(Cell));
			const AActor* Existing = Entry ? Entry->Get() : nullptr;
			if (IsValid(Existing) && Existing != ReplacedActor && !Existing->IsA<ABaseDrone>()) return false;
		}
	}
	return HasBuildingClearance(OriginCell, Footprint, Building);
}
bool APlanetSurfaceManager::ReserveCells(AActor* Occupier, FSTPGridCell OriginCell, FIntPoint Footprint)
{
	ABaseBuilding* BuildingOccupier = Cast<ABaseBuilding>(Occupier);
	if (!IsValid(Occupier))
	{
		LogPlacementDiagnostics(Occupier, OriginCell, Footprint, TEXT("ReserveCells rejected"));
		return false;
	}

	Footprint = SanitizeFootprint(Footprint);
	if (BuildingOccupier)
	{
		if (!CanReserveBuildingCells(BuildingOccupier, OriginCell, Footprint))
		{
			LogPlacementDiagnostics(Occupier, OriginCell, Footprint, TEXT("ReserveCells rejected"));
			return false;
		}
	}
	else if (!CanOccupyCells(OriginCell, Footprint))
	{
		LogPlacementDiagnostics(Occupier, OriginCell, Footprint, TEXT("ReserveCells rejected"));
		return false;
	}

	if (BuildingOccupier)
	{
		// Drones are mobile agents, not placement obstacles. Send any idle drone
		// standing inside the new footprint to a free cell beside the building.
		for (TActorIterator<ABaseDrone> It(GetWorld()); It; ++It)
		{
			ABaseDrone* Drone = *It;
			FSTPGridCell DroneCell;
			if (!IsValid(Drone) || !GetCellForWorldLocation(Drone->GetActorLocation(), DroneCell)
				|| DroneCell.X < OriginCell.X || DroneCell.X >= OriginCell.X + Footprint.X
				|| DroneCell.Y < OriginCell.Y || DroneCell.Y >= OriginCell.Y + Footprint.Y)
			{
				continue;
			}

			FSTPGridCell EvadeCell;
			FVector EvadeLocation;
			if (FindNearestFreeCellAdjacentToFootprint(
				OriginCell, Footprint, Drone->GetGridFootprint(), EvadeCell, EvadeLocation))
			{
				Drone->MoveAsideForConstruction(EvadeLocation);
			}
		}
	}

	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			OccupiedCells.Add(MakeCellKey(Cell), Occupier);
		}
	}

	if (BuildingOccupier)
	{
		bBuildingRoutingBoundsDirty = true;
		OnBuildingOccupancyChanged.Broadcast();
	}
	return true;
}

void APlanetSurfaceManager::LogPlacementDiagnostics(AActor* Occupier, FSTPGridCell OriginCell, FIntPoint Footprint, const TCHAR* Stage) const
{
	const FIntPoint Sanitized = SanitizeFootprint(Footprint);
	ABaseBuilding* Building = Cast<ABaseBuilding>(Occupier);
	const bool bClearance = HasBuildingClearance(OriginCell, Sanitized, Building, true);
	UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG stage=%s surface=%s actor=%s class=%s origin=(%d,%d) footprint=(%d,%d) sanitized=(%d,%d) HasBuildingClearance=%d ignored=%s"),
		Stage, *GetName(), *GetNameSafe(Occupier), *GetNameSafe(Occupier ? Occupier->GetClass() : nullptr), OriginCell.X, OriginCell.Y, Footprint.X, Footprint.Y, Sanitized.X, Sanitized.Y, bClearance, *GetNameSafe(Building));
	for (int32 Y = 0; Y < Sanitized.Y; ++Y)
	{
		for (int32 X = 0; X < Sanitized.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			const TObjectPtr<AActor>* Entry = OccupiedCells.Find(MakeCellKey(Cell));
			AActor* Existing = Entry ? Entry->Get() : nullptr;
			const bool bPlayable = IsCellPlayable(Cell);
			if (!bPlayable || Entry)
			{
				UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG cell=(%d,%d) playable=%d entry=%d actor=%s class=%s valid=%d mobileDrone=%d"),
					Cell.X, Cell.Y, bPlayable, Entry != nullptr, *GetNameSafe(Existing), *GetNameSafe(Existing ? Existing->GetClass() : nullptr), IsValid(Existing), IsValid(Existing) && Existing->IsA<ABaseDrone>());
			}
		}
	}
}
void APlanetSurfaceManager::ReleaseCells(AActor* Occupier)
{
	if (!Occupier)
	{
		return;
	}

	const ABaseBuilding* RemovedBuilding = Cast<ABaseBuilding>(Occupier);
	bool bBuildingRemoved = RemovedBuilding && !RemovedBuilding->IsPlacementPreview();
	for (auto It = OccupiedCells.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Value().Get()) || It.Value().Get() == Occupier)
		{
			bBuildingRemoved |= It.Value().Get() && It.Value()->IsA<ABaseBuilding>();
			It.RemoveCurrent();
		}
	}
	if (bBuildingRemoved)
	{
		bBuildingRoutingBoundsDirty = true;
		OnBuildingOccupancyChanged.Broadcast();
	}
}

void APlanetSurfaceManager::GetBuildingRoutingBounds(TMap<AActor*, FBox>& OutBounds) const
{
 // Share one snapshot across all links; moving previews never scan world actors.
 if (!bBuildingRoutingBoundsDirty)
 {
  OutBounds = BuildingRoutingBounds;
  return;
 }
 BuildingRoutingBounds.Reset();
 TerrainRoutingBounds.Reset();
 TerrainMeshRoutingBounds.Reset();
 const float HalfCell = TileSpacing * 0.5f;
 for (const auto& Entry : OccupiedCells)
 {
  ABaseBuilding* Building = Cast<ABaseBuilding>(Entry.Value.Get());
  if (!IsValid(Building) || Building->IsActorBeingDestroyed() || Building->IsPlacementPreview()) continue;
  FBox* Bounds = BuildingRoutingBounds.Find(Building);
  if (!Bounds) Bounds = &BuildingRoutingBounds.Add(Building, FBox(ForceInit));
  const FVector Center = GetWorldLocationForCell(FSTPGridCell(Entry.Key % GridWidth, Entry.Key / GridWidth));
  for (float X : {-HalfCell, HalfCell})
   for (float Y : {-HalfCell, HalfCell})
    *Bounds += Center + GetActorTransform().TransformVector(FVector(X, Y, 0));
 }
 // Placement already treats map-authored buildings as obstacles even without reservations.
 for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
 {
  ABaseBuilding* Building = *It;
  if (!IsValid(Building) || Building->IsActorBeingDestroyed() || Building->IsPlacementPreview()
   || BuildingRoutingBounds.Contains(Building)) continue;
  const FIntPoint Footprint = SanitizeFootprint(Building->GetGridFootprint());
  const FSTPGridPlacement Placement = GetPlacementForWorldLocation(Building->GetActorLocation(), Footprint);
  const FVector Center = GetWorldLocationForOriginCell(Placement.OriginCell, Footprint);
  FBox Bounds(ForceInit);
  for (float X : {-Footprint.X * HalfCell, Footprint.X * HalfCell})
   for (float Y : {-Footprint.Y * HalfCell, Footprint.Y * HalfCell})
    Bounds += Center + GetActorTransform().TransformVector(FVector(X, Y, 0));
  BuildingRoutingBounds.Add(Building, Bounds);
 }
 for (TActorIterator<APlanetGeneratedSector> It(GetWorld()); It; ++It)
 {
  if (It->IsActorBeingDestroyed()) continue;
  for (const FBox& MeshBounds : It->GetPlacementMeshBounds())
   TerrainMeshRoutingBounds.Add(MeshBounds.TransformBy(It->GetActorTransform()));
  for (const FBox& Cluster : It->GetPlacementClusterBounds())
   TerrainRoutingBounds.Add(Cluster.TransformBy(It->GetActorTransform()));
 }
 bBuildingRoutingBoundsDirty = false;
 OutBounds = BuildingRoutingBounds;
}


const TArray<FBox>& APlanetSurfaceManager::GetTerrainRoutingBounds() const
{
 if (bBuildingRoutingBoundsDirty)
 {
  TMap<AActor*, FBox> Unused;
  GetBuildingRoutingBounds(Unused);
 }
 return TerrainRoutingBounds;
}

void APlanetSurfaceManager::InvalidateRoutingObstacles()
{
 bBuildingRoutingBoundsDirty = true;
 // Procedural population can create many sector shells in one frame. Refresh links once.
 if (bRoutingRefreshPending || !GetWorld()->IsGameWorld() || GetWorld()->bIsTearingDown) return;
 bRoutingRefreshPending = true;
 GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
 {
  bRoutingRefreshPending = false;
  OnBuildingOccupancyChanged.Broadcast();
 }));
}

bool APlanetSurfaceManager::TryGetActorOriginCell(AActor* Actor, FSTPGridCell& OutOriginCell) const
{
	if (!Actor)
	{
		return false;
	}

	bool bFound = false;
	int32 BestX = MAX_int32;
	int32 BestY = MAX_int32;

	for (const TPair<int32, TObjectPtr<AActor>>& Pair : OccupiedCells)
	{
		if (Pair.Value.Get() != Actor)
		{
			continue;
		}

		const int32 X = Pair.Key % GridWidth;
		const int32 Y = Pair.Key / GridWidth;
		BestX = FMath::Min(BestX, X);
		BestY = FMath::Min(BestY, Y);
		bFound = true;
	}

	if (bFound)
	{
		OutOriginCell = FSTPGridCell(BestX, BestY);
	}

	return bFound;
}
bool APlanetSurfaceManager::FindNearestFreeCellAdjacentToActor(AActor* Actor, FIntPoint Footprint, FSTPGridCell& OutCell, FVector& OutWorldLocation) const
{
	TArray<FSTPGridCell> ActorCells;
	if (!FindOccupiedCellsForActor(Actor, ActorCells))
	{
		return false;
	}

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;
	int32 MaxX = MIN_int32;
	int32 MaxY = MIN_int32;

	for (const FSTPGridCell& Cell : ActorCells)
	{
		MinX = FMath::Min(MinX, Cell.X);
		MinY = FMath::Min(MinY, Cell.Y);
		MaxX = FMath::Max(MaxX, Cell.X);
		MaxY = FMath::Max(MaxY, Cell.Y);
	}

	Footprint = SanitizeFootprint(Footprint);
	bool bFound = false;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (int32 Y = MinY - 1; Y <= MaxY + 1; ++Y)
	{
		for (int32 X = MinX - 1; X <= MaxX + 1; ++X)
		{
			const bool bInsideActorBounds = X >= MinX && X <= MaxX && Y >= MinY && Y <= MaxY;
			if (bInsideActorBounds)
			{
				continue;
			}

			const FSTPGridCell CandidateCell(X, Y);
			if (!CanOccupyCells(CandidateCell, Footprint))
			{
				continue;
			}

			const FVector CandidateWorldLocation = GetWorldLocationForOriginCell(CandidateCell, Footprint);
			const float DistanceSq = FVector::DistSquared(Actor->GetActorLocation(), CandidateWorldLocation);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				OutCell = CandidateCell;
				OutWorldLocation = CandidateWorldLocation;
				bFound = true;
			}
		}
	}

	return bFound;
}

bool APlanetSurfaceManager::FindNearestFreeCellAdjacentToFootprint(FSTPGridCell OriginCell, FIntPoint OccupiedFootprint, FIntPoint SearchFootprint, FSTPGridCell& OutCell, FVector& OutWorldLocation) const
{
	OccupiedFootprint = SanitizeFootprint(OccupiedFootprint);
	SearchFootprint = SanitizeFootprint(SearchFootprint);

	const int32 MinX = OriginCell.X;
	const int32 MinY = OriginCell.Y;
	const int32 MaxX = OriginCell.X + OccupiedFootprint.X - 1;
	const int32 MaxY = OriginCell.Y + OccupiedFootprint.Y - 1;
	const FVector OccupiedWorldLocation = GetWorldLocationForOriginCell(OriginCell, OccupiedFootprint);

	bool bFound = false;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (int32 Y = MinY - SearchFootprint.Y; Y <= MaxY + 1; ++Y)
	{
		for (int32 X = MinX - SearchFootprint.X; X <= MaxX + 1; ++X)
		{
			const bool bInsideOccupiedFootprint = X <= MaxX && X + SearchFootprint.X - 1 >= MinX
				&& Y <= MaxY && Y + SearchFootprint.Y - 1 >= MinY;
			if (bInsideOccupiedFootprint)
			{
				continue;
			}

			const FSTPGridCell CandidateCell(X, Y);
			if (!CanOccupyCells(CandidateCell, SearchFootprint))
			{
				continue;
			}

			const FVector CandidateWorldLocation = GetWorldLocationForOriginCell(CandidateCell, SearchFootprint);
			const float DistanceSq = FVector::DistSquared(OccupiedWorldLocation, CandidateWorldLocation);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				OutCell = CandidateCell;
				OutWorldLocation = CandidateWorldLocation;
				bFound = true;
			}
		}
	}

	return bFound;
}

bool APlanetSurfaceManager::FindGridPath(const FVector& StartWorldLocation, FSTPGridCell GoalCell, FIntPoint Footprint, TArray<FVector>& OutWorldPath) const
{
	OutWorldPath.Reset();
	Footprint = SanitizeFootprint(Footprint);
	if (!IsCellInBounds(GoalCell) || !CanOccupyCells(GoalCell, Footprint))
	{
		return false;
	}

	const FIntPoint Start = GetPlacementForWorldLocation(StartWorldLocation, Footprint).OriginCell.ToIntPoint();
	const FIntPoint Goal = GoalCell.ToIntPoint();
	if (Start == Goal)
	{
		OutWorldPath.Add(GetWorldLocationForOriginCell(GoalCell, Footprint));
		return true;
	}

	TSet<FIntPoint> OpenSet;
	TSet<FIntPoint> ClosedSet;
	TMap<FIntPoint, FIntPoint> CameFrom;
	TMap<FIntPoint, int32> GScore;
	OpenSet.Add(Start);
	GScore.Add(Start, 0);

	const FIntPoint Directions[] = {
		FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)
	};

	while (!OpenSet.IsEmpty())
	{
		FIntPoint Current = *OpenSet.CreateConstIterator();
		int32 BestScore = MAX_int32;
		for (const FIntPoint& Candidate : OpenSet)
		{
			const int32 CandidateG = GScore.FindRef(Candidate);
			const int32 CandidateF = CandidateG + FMath::Abs(Candidate.X - Goal.X) + FMath::Abs(Candidate.Y - Goal.Y);
			if (CandidateF < BestScore)
			{
				BestScore = CandidateF;
				Current = Candidate;
			}
		}

		if (Current == Goal)
		{
			TArray<FIntPoint> ReversePath;
			while (Current != Start)
			{
				ReversePath.Add(Current);
				const FIntPoint* Previous = CameFrom.Find(Current);
				if (!Previous)
				{
					return false;
				}
				Current = *Previous;
			}

			for (int32 Index = ReversePath.Num() - 1; Index >= 0; --Index)
			{
				OutWorldPath.Add(GetWorldLocationForOriginCell(FSTPGridCell(ReversePath[Index]), Footprint));
			}
			return !OutWorldPath.IsEmpty();
		}

		OpenSet.Remove(Current);
		ClosedSet.Add(Current);
		for (const FIntPoint& Direction : Directions)
		{
			const FIntPoint Neighbor = Current + Direction;
			if (ClosedSet.Contains(Neighbor) || !CanOccupyCells(FSTPGridCell(Neighbor), Footprint))
			{
				continue;
			}

			const int32 TentativeG = GScore.FindRef(Current) + 1;
			const int32* ExistingG = GScore.Find(Neighbor);
			if (!ExistingG || TentativeG < *ExistingG)
			{
				CameFrom.Add(Neighbor, Current);
				GScore.Add(Neighbor, TentativeG);
				OpenSet.Add(Neighbor);
			}
		}
	}

	return false;
}
void APlanetSurfaceManager::SpawnSurface()
{
	SpawnChunks();
}

void APlanetSurfaceManager::SpawnChunks()
{
	if (ChunkMeshes.IsEmpty() || !GetRootComponent())
	{
		return;
	}

	TArray<UStaticMesh*> ValidMeshes;
	for (UStaticMesh* ChunkMesh : ChunkMeshes)
	{
		if (IsValid(ChunkMesh))
		{
			ValidMeshes.Add(ChunkMesh);
		}
	}

	if (ValidMeshes.IsEmpty())
	{
		return;
	}

	const float ChunkWorldSize = CellsPerChunk * TileSpacing;
	const float MeshScale = ChunkWorldSize / FMath::Max(1.0f, ChunkMeshNativeSize);
	const float HalfDiameter = (ChunkDiameter - 1) * 0.5f;
	FRandomStream RandomStream(ChunkRandomSeed);

	for (int32 ChunkY = 0; ChunkY < ChunkDiameter; ++ChunkY)
	{
		for (int32 ChunkX = 0; ChunkX < ChunkDiameter; ++ChunkX)
		{
			if (!IsChunkInWorld(ChunkX, ChunkY))
			{
				continue;
			}

			const int32 MeshIndex = RandomStream.RandRange(0, ValidMeshes.Num() - 1);
			const FName ComponentName = MakeUniqueObjectName(
				this,
				UStaticMeshComponent::StaticClass(),
				*FString::Printf(TEXT("SurfaceChunk_%d_%d"), ChunkX, ChunkY));

			UStaticMeshComponent* ChunkComponent = NewObject<UStaticMeshComponent>(
				this,
				UStaticMeshComponent::StaticClass(),
				ComponentName,
				RF_Transactional);

			ChunkComponent->SetupAttachment(GetRootComponent());
			ChunkComponent->SetStaticMesh(ValidMeshes[MeshIndex]);
			if (ChunkMaterial)
			{
				ChunkComponent->SetMaterial(0, ChunkMaterial);
			}
			ChunkComponent->SetMobility(EComponentMobility::Static);
			ChunkComponent->SetGenerateOverlapEvents(false);
			ChunkComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ChunkComponent->SetRelativeLocation(FVector(
				(ChunkX - HalfDiameter) * ChunkWorldSize,
				(ChunkY - HalfDiameter) * ChunkWorldSize,
				ChunkHeightOffset));
			ChunkComponent->SetRelativeScale3D(FVector(MeshScale, MeshScale, 1.0f));

			AddInstanceComponent(ChunkComponent);
			ChunkComponent->RegisterComponent();
			SpawnedChunkComponents.Add(ChunkComponent);
		}
	}
}

void APlanetSurfaceManager::ClearChunkComponents()
{
	for (UStaticMeshComponent* ChunkComponent : SpawnedChunkComponents)
	{
		if (IsValid(ChunkComponent))
		{
			RemoveInstanceComponent(ChunkComponent);
			ChunkComponent->DestroyComponent();
		}
	}

	SpawnedChunkComponents.Reset();
}

void APlanetSurfaceManager::UpdateDerivedGridSize()
{
	CellsPerChunk = FMath::Max(1, CellsPerChunk);
	ChunkDiameter = FMath::Max(1, ChunkDiameter);
	GridWidth = CellsPerChunk * ChunkDiameter;
	GridHeight = GridWidth;
}

bool APlanetSurfaceManager::IsChunkInWorld(int32 ChunkX, int32 ChunkY) const
{
	if (ChunkX < 0 || ChunkY < 0 || ChunkX >= ChunkDiameter || ChunkY >= ChunkDiameter)
	{
		return false;
	}

	const float Center = (ChunkDiameter - 1) * 0.5f;
	const float DeltaX = ChunkX - Center;
	const float DeltaY = ChunkY - Center;
	const float Radius = ChunkDiameter * 0.5f;
	return (DeltaX * DeltaX) + (DeltaY * DeltaY) <= Radius * Radius;
}

bool APlanetSurfaceManager::IsCellPlayable(FSTPGridCell Cell) const
{
	if (!IsCellInBounds(Cell))
	{
		return false;
	}

	const int32 ChunkX = Cell.X / FMath::Max(1, CellsPerChunk);
	const int32 ChunkY = Cell.Y / FMath::Max(1, CellsPerChunk);
	return IsChunkInWorld(ChunkX, ChunkY);
}

FVector2D APlanetSurfaceManager::GetGridOffset() const
{
	return FVector2D(
		bCenterGridOnActor ? (GridWidth - 1) * TileSpacing * 0.5f : 0.0f,
		bCenterGridOnActor ? (GridHeight - 1) * TileSpacing * 0.5f : 0.0f);
}

FVector APlanetSurfaceManager::GetWorldLocationForOriginCell(FSTPGridCell OriginCell, FIntPoint Footprint) const
{
	Footprint = SanitizeFootprint(Footprint);
	const FVector2D Offset = GetGridOffset();
	const float CenterGridX = OriginCell.X + ((Footprint.X - 1) * 0.5f);
	const float CenterGridY = OriginCell.Y + ((Footprint.Y - 1) * 0.5f);

	const FVector LocalLocation(
		(CenterGridX * TileSpacing) - Offset.X,
		(CenterGridY * TileSpacing) - Offset.Y,
		0.0f);

	return GetActorTransform().TransformPosition(LocalLocation);
}

int32 APlanetSurfaceManager::MakeCellKey(FSTPGridCell Cell) const
{
	return Cell.X + (Cell.Y * GridWidth);
}

FIntPoint APlanetSurfaceManager::SanitizeFootprint(FIntPoint Footprint) const
{
	return FIntPoint(FMath::Max(1, Footprint.X), FMath::Max(1, Footprint.Y));
}
bool APlanetSurfaceManager::FindOccupiedCellsForActor(AActor* Actor, TArray<FSTPGridCell>& OutCells) const
{
	OutCells.Reset();
	if (!Actor)
	{
		return false;
	}

	for (const TPair<int32, TObjectPtr<AActor>>& Pair : OccupiedCells)
	{
		if (Pair.Value.Get() == Actor)
		{
			const int32 X = Pair.Key % GridWidth;
			const int32 Y = Pair.Key / GridWidth;
			OutCells.Add(FSTPGridCell(X, Y));
		}
	}

	if (OutCells.IsEmpty())
	{
		if (const ABaseBuilding* Building = Cast<ABaseBuilding>(Actor))
		{
			const FIntPoint BuildingFootprint = SanitizeFootprint(Building->GetGridFootprint());
			const FSTPGridPlacement Placement = GetPlacementForWorldLocation(
				Building->GetActorLocation(), BuildingFootprint);
			for (int32 Y = 0; Y < BuildingFootprint.Y; ++Y)
			{
				for (int32 X = 0; X < BuildingFootprint.X; ++X)
				{
					const FSTPGridCell Cell(Placement.OriginCell.X + X, Placement.OriginCell.Y + Y);
					if (IsCellInBounds(Cell))
					{
						OutCells.Add(Cell);
					}
				}
			}
		}
	}

	return OutCells.Num() > 0;
}

bool APlanetSurfaceManager::CanBuildInSector(const ABaseBuilding* Building, const FVector& WorldLocation) const
{
	if (!Building) return false;
	for (TActorIterator<AHexSectorGrid> It(GetWorld()); It; ++It)
	{
		const FIntPoint Footprint = Building->GetGridFootprint();
		const FVector HalfX = GetActorTransform().TransformVector(FVector(Footprint.X * TileSpacing * 0.5f, 0, 0));
		const FVector HalfY = GetActorTransform().TransformVector(FVector(0, Footprint.Y * TileSpacing * 0.5f, 0));
		return It->CanPlaceBuilding(Building, WorldLocation, HalfX, HalfY);
	}
	// Legacy and isolated asset-preview maps do not have exploration sectors.
	return true;
}
