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
	EnsureDefaultTemplates();
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
	AssignTemplates(LayoutSeed);
}

void AHexSectorGrid::EnsureDefaultTemplates()
{
	if (!SectorTemplates.IsEmpty()) return;
	const FName Names[] = {TEXT("RockyBasin"), TEXT("SplitRidge"), TEXT("CraterShelf"),
		TEXT("TwinMesa"), TEXT("WindChannel"), TEXT("BrokenPlateau")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		FSectorTemplateDefinition Template;
		Template.TemplateId = Names[Index];
		Template.DressingRuleTags = {Index % 2 == 0 ? TEXT("RockScatter") : TEXT("RidgeScatter"), TEXT("KeepCorridorsClear")};
		Template.bSupportsHQ = Index == 0 || Index == 3;
		Template.BuildablePockets.Add({FVector2D(-650.0f + Index * 75.0f, 150.0f), FVector2D(900.0f, 700.0f)});
		Template.BuildablePockets.Add({FVector2D(850.0f, -500.0f + Index * 60.0f), FVector2D(550.0f, 450.0f)});
		Template.ResourceSlots.Add({TEXT("Any"), FVector2D(-1450.0f, -800.0f + Index * 120.0f), 300.0f});
		Template.ResourceSlots.Add({Index % 2 == 0 ? TEXT("Mineral") : TEXT("Water"), FVector2D(1350.0f, 700.0f), 250.0f});
		Template.LandmarkSlots.Add(FVector2D(0.0f, 1250.0f));
		Template.SubBaseSlots.Add(FVector2D(600.0f, 250.0f));
		for (int32 Side = 0; Side < 6; ++Side)
		{
			const float Angle = FMath::DegreesToRadians(30.0f + Side * 60.0f);
			Template.ConnectionSockets.Add(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ExplorationSectorRadius);
			Template.DroneCorridors.Add({FVector2D::ZeroVector,
				FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ExplorationSectorRadius, 350.0f});
		}
		SectorTemplates.Add(MoveTemp(Template));
	}
}

void AHexSectorGrid::AssignTemplates(int32 Seed)
{
	EnsureDefaultTemplates();
	if (SectorTemplates.IsEmpty()) return;
	FRandomStream Random(Seed);
	TArray<int32> Order;
	for (int32 Index = 0; Index < SectorTemplates.Num(); ++Index) Order.Add(Index);
	for (int32 Index = Order.Num() - 1; Index > 0; --Index) Order.Swap(Index, Random.RandRange(0, Index));
	int32 HQTemplateIndex = SectorTemplates.IndexOfByPredicate([](const FSectorTemplateDefinition& T) { return T.bSupportsHQ; });
	if (HQTemplateIndex == INDEX_NONE) HQTemplateIndex = 0;
	Order.Remove(HQTemplateIndex);
	int32 NonStartTemplate = 0;

	for (FHexSector& Sector : Sectors)
	{
		const int32 TemplateIndex = Sector.Id == StartingSectorId || Order.IsEmpty()
			? HQTemplateIndex : Order[(NonStartTemplate++) % Order.Num()];
		const FSectorTemplateDefinition& Template = SectorTemplates[TemplateIndex];
		Sector.TemplateId = Template.TemplateId;
		Sector.TemplateRotationDegrees = Template.SafeRotations.IsEmpty()
			? 0 : Template.SafeRotations[Random.RandRange(0, Template.SafeRotations.Num() - 1)];
		Sector.bTemplateMirrored = Template.bAllowMirroring && Random.RandRange(0, 1) == 1;
	}
}

bool AHexSectorGrid::GetTemplateForSector(int32 SectorId, FSectorTemplateDefinition& OutTemplate) const
{
	FHexSector Sector;
	if (!GetSectorById(SectorId, Sector)) return false;
	const FSectorTemplateDefinition* Found = SectorTemplates.FindByPredicate([&Sector](const FSectorTemplateDefinition& T)
	{
		return T.TemplateId == Sector.TemplateId;
	});
	if (!Found) return false;
	OutTemplate = *Found;
	return true;
}

bool AHexSectorGrid::ValidateGeneratedLayout(FString& OutDiagnostic) const
{
	if (SectorTemplates.Num() < 6) { OutDiagnostic = TEXT("At least six sector templates are required."); return false; }
	FHexSector Start;
	FSectorTemplateDefinition StartTemplate;
	if (!GetSectorById(StartingSectorId, Start) || !GetTemplateForSector(StartingSectorId, StartTemplate)
		|| !StartTemplate.bSupportsHQ || StartTemplate.BuildablePockets.IsEmpty())
	{
		OutDiagnostic = TEXT("Starting sector has no HQ-capable buildable template.");
		return false;
	}
	for (const FHexSector& Sector : Sectors)
	{
		FSectorTemplateDefinition Template;
		if (!GetTemplateForSector(Sector.Id, Template) || Template.ConnectionSockets.Num() < 6
			|| Template.DroneCorridors.Num() < 6)
		{
			OutDiagnostic = FString::Printf(TEXT("Sector %d lacks a template or connection corridors."), Sector.Id);
			return false;
		}
	}
	OutDiagnostic = TEXT("Layout is valid.");
	return true;
}

FVector AHexSectorGrid::TransformTemplatePoint(const FHexSector& Sector, FVector2D Point) const
{
	if (Sector.bTemplateMirrored) Point.X *= -1.0f;
	Point = Point.GetRotated(Sector.TemplateRotationDegrees);
	return Sector.WorldCenter + GetActorTransform().TransformVectorNoScale(FVector(Point, 0.0f));
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
		if (bShowTemplateDebug)
		{
			FSectorTemplateDefinition Template;
			if (!GetTemplateForSector(Sector.Id, Template)) continue;
			DrawDebugString(GetWorld(), Sector.WorldCenter + FVector(0, 0, 120),
				FString::Printf(TEXT("S%d %s R%d%s"), Sector.Id, *Sector.TemplateId.ToString(),
					Sector.TemplateRotationDegrees, Sector.bTemplateMirrored ? TEXT(" M") : TEXT("")),
				nullptr, SectorColor, 0.0f, true);
			for (const FSectorBuildablePocket& Pocket : Template.BuildablePockets)
			{
				DrawDebugBox(GetWorld(), TransformTemplatePoint(Sector, Pocket.Center), FVector(Pocket.Extent, 15.0f),
					FColor::Green, false, 0.0f, 0, 4.0f);
			}
			for (const FSectorResourceSlot& Slot : Template.ResourceSlots)
			{
				DrawDebugSphere(GetWorld(), TransformTemplatePoint(Sector, Slot.Location), Slot.Radius, 12,
					FColor::Yellow, false, 0.0f, 0, 3.0f);
			}
			for (const FVector2D& Socket : Template.ConnectionSockets)
			{
				DrawDebugSphere(GetWorld(), TransformTemplatePoint(Sector, Socket), 90.0f, 8,
					FColor::Magenta, false, 0.0f, 0, 4.0f);
			}
			for (const FSectorCorridor& Corridor : Template.DroneCorridors)
			{
				DrawDebugLine(GetWorld(), TransformTemplatePoint(Sector, Corridor.Start),
					TransformTemplatePoint(Sector, Corridor.End), FColor::Cyan, false, 0.0f, 0, Corridor.Width / 20.0f);
			}
		}
	}
}
