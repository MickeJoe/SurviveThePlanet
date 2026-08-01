#include "PlanetSurfaceManager.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Gameplay/Base/BaseBuilding.h"

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
