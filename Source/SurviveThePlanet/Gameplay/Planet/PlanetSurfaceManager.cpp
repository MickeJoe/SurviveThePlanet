#include "PlanetSurfaceManager.h"

#include "Engine/World.h"

APlanetSurfaceManager::APlanetSurfaceManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APlanetSurfaceManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

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
	ClearSurface();
	SpawnSurface();
}

void APlanetSurfaceManager::ClearSurface()
{
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

bool APlanetSurfaceManager::GetCellForWorldLocation(const FVector& WorldLocation, FSTPGridCell& OutCell) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FVector2D Offset = GetGridOffset();

	OutCell = FSTPGridCell(
		FMath::RoundToInt((LocalLocation.X + Offset.X) / TileSpacing),
		FMath::RoundToInt((LocalLocation.Y + Offset.Y) / TileSpacing));

	return IsCellInBounds(OutCell);
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

	if (OriginCell.X < 0 || OriginCell.Y < 0)
	{
		return false;
	}

	if (OriginCell.X + Footprint.X > GridWidth || OriginCell.Y + Footprint.Y > GridHeight)
	{
		return false;
	}

	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			const TObjectPtr<AActor>* ExistingOccupier = OccupiedCells.Find(MakeCellKey(Cell));
			if (ExistingOccupier && IsValid(ExistingOccupier->Get()))
			{
				return false;
			}
		}
	}

	return true;
}

bool APlanetSurfaceManager::ReserveCells(AActor* Occupier, FSTPGridCell OriginCell, FIntPoint Footprint)
{
	if (!IsValid(Occupier) || !CanOccupyCells(OriginCell, Footprint))
	{
		return false;
	}

	Footprint = SanitizeFootprint(Footprint);
	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const FSTPGridCell Cell(OriginCell.X + X, OriginCell.Y + Y);
			OccupiedCells.Add(MakeCellKey(Cell), Occupier);
		}
	}

	return true;
}

void APlanetSurfaceManager::ReleaseCells(AActor* Occupier)
{
	if (!Occupier)
	{
		return;
	}

	for (auto It = OccupiedCells.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Value().Get()) || It.Value().Get() == Occupier)
		{
			It.RemoveCurrent();
		}
	}
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

	for (int32 Y = MinY - 1; Y <= MaxY + 1; ++Y)
	{
		for (int32 X = MinX - 1; X <= MaxX + 1; ++X)
		{
			const bool bInsideOccupiedFootprint = X >= MinX && X <= MaxX && Y >= MinY && Y <= MaxY;
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
void APlanetSurfaceManager::SpawnSurface()
{
	if (!TileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform ManagerTransform = GetActorTransform();

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FVector LocalLocation = GetTileLocation(X, Y);
			const FVector WorldTileLocation = ManagerTransform.TransformPosition(LocalLocation);

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* Tile = World->SpawnActor<AActor>(
				TileClass,
				WorldTileLocation,
				GetActorRotation(),
				SpawnParameters);

			if (Tile)
			{
				Tile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				SpawnedTiles.Add(Tile);
			}
		}
	}
}

FVector APlanetSurfaceManager::GetTileLocation(int32 X, int32 Y) const
{
	const FVector2D Offset = GetGridOffset();

	return FVector(
		(X * TileSpacing) - Offset.X,
		(Y * TileSpacing) - Offset.Y,
		0.0f);
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

	return OutCells.Num() > 0;
}
