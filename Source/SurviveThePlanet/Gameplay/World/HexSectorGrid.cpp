#include "Gameplay/World/HexSectorGrid.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

namespace HexSector
{
	const FIntPoint Directions[6] = {{1,0},{1,-1},{0,-1},{-1,0},{-1,1},{0,1}};

	int32 RingFor(int32 Q, int32 R)
	{
		return FMath::Max3(FMath::Abs(Q), FMath::Abs(R), FMath::Abs(-Q - R));
	}

	int32 IdFor(int32 Q, int32 R, int32 GridRadius)
	{
		if (Q == 0 && R == 0) return 0;
		int32 Id = 1;
		for (int32 Ring = 1; Ring <= GridRadius; ++Ring)
		{
			int32 CQ = -Ring;
			int32 CR = Ring;
			for (int32 Side = 0; Side < 6; ++Side)
			{
				for (int32 Step = 0; Step < Ring; ++Step, ++Id)
				{
					if (CQ == Q && CR == R) return Id;
					CQ += Directions[Side].X;
					CR += Directions[Side].Y;
				}
			}
		}
		return INDEX_NONE;
	}
}

AHexSectorGrid::AHexSectorGrid()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickInterval(0.0f);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
}

void AHexSectorGrid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildGrid();
}

void AHexSectorGrid::BeginPlay()
{
	Super::BeginPlay();
	RebuildGrid();
}

void AHexSectorGrid::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bShowWorldGrid) DrawGrid();
}

void AHexSectorGrid::RebuildGrid()
{
	const int32 ClampedGridRadius = FMath::Max(0, GridRadius);
	Sectors.Reset(1 + 3 * ClampedGridRadius * (ClampedGridRadius + 1));
	for (int32 Q = -ClampedGridRadius; Q <= ClampedGridRadius; ++Q)
	{
		for (int32 R = FMath::Max(-ClampedGridRadius, -Q - ClampedGridRadius);
			R <= FMath::Min(ClampedGridRadius, -Q + ClampedGridRadius); ++R)
		{
			FHexSector Sector;
			Sector.Id = HexSector::IdFor(Q, R, ClampedGridRadius);
			Sector.Q = Q;
			Sector.R = R;
			Sector.Ring = HexSector::RingFor(Q, R);
			Sector.WorldCenter = AxialToWorld(Q, R);
			Sector.State = bEstablishStartingSector && Sector.Id == StartingSectorId
				? ESectorState::Established
				: ESectorState::Undiscovered;
			for (const FIntPoint& Direction : HexSector::Directions)
			{
				const int32 NeighborId = HexSector::IdFor(Q + Direction.X, R + Direction.Y, ClampedGridRadius);
				if (NeighborId != INDEX_NONE) Sector.NeighborIds.Add(NeighborId);
			}
			Sectors.Add(MoveTemp(Sector));
		}
	}
	Sectors.Sort([](const FHexSector& A, const FHexSector& B) { return A.Id < B.Id; });
}

FVector AHexSectorGrid::AxialToWorld(int32 Q, int32 R) const
{
	const FVector Local(FMath::Sqrt(3.0f) * ExplorationSectorRadius * (Q + R * 0.5f), ExplorationSectorRadius * 1.5f * R, LineHeightOffset);
	return GetActorTransform().TransformPosition(Local);
}

FIntPoint AHexSectorGrid::RoundAxial(const FVector2D& Local) const
{
	const float Qf = (FMath::Sqrt(3.0f) / 3.0f * Local.X - Local.Y / 3.0f) / ExplorationSectorRadius;
	const float Rf = (2.0f / 3.0f * Local.Y) / ExplorationSectorRadius;
	float X = Qf, Z = Rf, Y = -X - Z;
	int32 RX = FMath::RoundToInt(X), RY = FMath::RoundToInt(Y), RZ = FMath::RoundToInt(Z);
	const float DX = FMath::Abs(RX - X), DY = FMath::Abs(RY - Y), DZ = FMath::Abs(RZ - Z);
	if (DX > DY && DX > DZ) RX = -RY - RZ;
	else if (DY > DZ) RY = -RX - RZ;
	else RZ = -RX - RY;
	return FIntPoint(RX, RZ);
}

int32 AHexSectorGrid::GetSectorAtWorldLocation(FVector WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FIntPoint Axial = RoundAxial(FVector2D(Local.X, Local.Y));
	return HexSector::IdFor(Axial.X, Axial.Y, FMath::Max(0, GridRadius));
}

bool AHexSectorGrid::GetSectorById(int32 SectorId, FHexSector& OutSector) const
{
	if (const FHexSector* Found = Sectors.FindByPredicate([SectorId](const FHexSector& S) { return S.Id == SectorId; }))
	{
		OutSector = *Found;
		return true;
	}
	return false;
}

TArray<int32> AHexSectorGrid::GetNeighborIds(int32 SectorId) const
{
	FHexSector Sector;
	return GetSectorById(SectorId, Sector) ? Sector.NeighborIds : TArray<int32>();
}

TArray<int32> AHexSectorGrid::GetNearestUndiscoveredSectorIds(FVector Origin, int32 MaxCount) const
{
	TArray<const FHexSector*> Candidates;
	for (const FHexSector& Sector : Sectors)
	{
		if (Sector.State == ESectorState::Undiscovered)
		{
			Candidates.Add(&Sector);
		}
	}

	Candidates.Sort([Origin](const FHexSector& A, const FHexSector& B)
	{
		return FVector::DistSquared2D(Origin, A.WorldCenter) < FVector::DistSquared2D(Origin, B.WorldCenter);
	});

	TArray<int32> Result;
	const int32 Count = FMath::Min(FMath::Max(0, MaxCount), Candidates.Num());
	Result.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Result.Add(Candidates[Index]->Id);
	}
	return Result;
}

bool AHexSectorGrid::SetSectorState(int32 SectorId, ESectorState NewState)
{
	if (FHexSector* Sector = Sectors.FindByPredicate([SectorId](const FHexSector& Candidate) { return Candidate.Id == SectorId; }))
	{
		if (Sector->State == NewState)
		{
			return false;
		}
		Sector->State = NewState;
		OnSectorStateChanged.Broadcast(SectorId, NewState);
		return true;
	}
	return false;
}

float AHexSectorGrid::GetFogOpacityForSector(int32 SectorId) const
{
	const FHexSector* Sector = Sectors.FindByPredicate([SectorId](const FHexSector& Candidate) { return Candidate.Id == SectorId; });
	if (!Sector)
	{
		return 1.0f;
	}

	switch (Sector->State)
	{
	case ESectorState::Established:
		return 0.0f;
	case ESectorState::Discovered:
		return FMath::Clamp(DiscoveredFogOpacity, 0.0f, 1.0f);
	case ESectorState::Undiscovered:
	default:
		return FMath::Clamp(UndiscoveredFogOpacity, 0.0f, 1.0f);
	}
}

FColor AHexSectorGrid::GetDebugColor(ESectorState State) const
{
	switch (State)
	{
	case ESectorState::Discovered:
		return DiscoveredColor.ToFColor(true);
	case ESectorState::Established:
		return EstablishedColor.ToFColor(true);
	case ESectorState::Undiscovered:
	default:
		return UndiscoveredColor.ToFColor(true);
	}
}

void AHexSectorGrid::DrawGrid() const
{
	for (const FHexSector& Sector : Sectors)
	{
		const FColor SectorColor = GetDebugColor(Sector.State);
		FVector Corners[6];
		for (int32 Corner = 0; Corner < 6; ++Corner)
		{
			const float Angle = FMath::DegreesToRadians(30.0f + Corner * 60.0f);
			Corners[Corner] = Sector.WorldCenter + GetActorTransform().TransformVectorNoScale(
				FVector(FMath::Cos(Angle) * ExplorationSectorRadius, FMath::Sin(Angle) * ExplorationSectorRadius, 0.0f));
		}
		for (int32 Corner = 0; Corner < 6; ++Corner)
		{
			DrawDebugLine(GetWorld(), Corners[Corner], Corners[(Corner + 1) % 6], SectorColor, false, 0.0f, 0, LineThickness);
		}
	}
}
