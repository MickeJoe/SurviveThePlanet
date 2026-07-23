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