#include "PlanetSectorTemplate.h"
#include "PlanetTerrainClusterShape.h"

namespace
{
	bool PointOnEdge(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Edge = B - A;
		const double Alpha = Edge.SizeSquared() > UE_SMALL_NUMBER
			? FMath::Clamp(FVector2D::DotProduct(P - A, Edge) / Edge.SizeSquared(), 0.0, 1.0) : 0.0;
		return (P - (A + Alpha * Edge)).SizeSquared() <= 0.0001;
	}

	bool ContainsPoint(TConstArrayView<FVector2D> Polygon, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 I = 0, J = Polygon.Num() - 1; I < Polygon.Num(); J = I++)
		{
			const FVector2D& A = Polygon[J];
			const FVector2D& B = Polygon[I];
			if (PointOnEdge(Point, A, B)) return true;
			if ((A.Y > Point.Y) != (B.Y > Point.Y)
				&& Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X) bInside = !bInside;
		}
		return bInside;
	}

	bool EdgesIntersect(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		if (PointOnEdge(A, C, D) || PointOnEdge(B, C, D) || PointOnEdge(C, A, B) || PointOnEdge(D, A, B)) return true;
		const double AB_C = FVector2D::CrossProduct(B - A, C - A);
		const double AB_D = FVector2D::CrossProduct(B - A, D - A);
		const double CD_A = FVector2D::CrossProduct(D - C, A - C);
		const double CD_B = FVector2D::CrossProduct(D - C, B - C);
		return AB_C * AB_D < 0.0 && CD_A * CD_B < 0.0;
	}
}

bool UPlanetSectorTemplate::OverlapsBuildingFootprint(TConstArrayView<FVector> WorldFootprint,
	const FTransform& SectorTransform) const
{
	if (WorldFootprint.Num() < 3) return false;
	for (const FPlanetSectorClusterSlot& Slot : ClusterSlots)
	{
		if (!Slot.Shape || Slot.Shape->Points.Num() < 3) continue;
		const TArray<FVector2D>& Polygon = Slot.Shape->Points;
		const FTransform ShapeTransform = Slot.Transform * SectorTransform;
		TArray<FVector2D, TInlineAllocator<4>> Candidate;
		FBox2D CandidateBounds(ForceInit), ShapeBounds(ForceInit);
		for (const FVector& Corner : WorldFootprint)
		{
			const FVector Local = ShapeTransform.InverseTransformPosition(Corner);
			const FVector2D Point(Local.X, Local.Y);
			Candidate.Add(Point);
			CandidateBounds += Point;
		}
		for (const FVector2D& Point : Polygon) ShapeBounds += Point;
		if (!CandidateBounds.ExpandBy(0.01).Intersect(ShapeBounds)) continue;
		// Containment in either direction plus edge crossings handles concave shapes,
		// small islands enclosed by a building, and thin strips crossing its middle.
		for (const FVector2D& Point : Candidate) if (ContainsPoint(Polygon, Point)) return true;
		for (const FVector2D& Point : Polygon) if (ContainsPoint(Candidate, Point)) return true;
		for (int32 I = 0; I < Candidate.Num(); ++I)
		{
			for (int32 J = 0; J < Polygon.Num(); ++J)
			{
				if (EdgesIntersect(Candidate[I], Candidate[(I + 1) % Candidate.Num()],
					Polygon[J], Polygon[(J + 1) % Polygon.Num()])) return true;
			}
		}
	}
	return false;
}

UPlanetSectorTemplate* UPlanetSectorTemplate::SelectForSector(const TArray<UPlanetSectorTemplate*>& Templates,
	int32 WorldSeed, int32 SectorId, bool bStartingSector)
{
	TArray<UPlanetSectorTemplate*> Eligible;
	for (UPlanetSectorTemplate* Template : Templates)
	{
		if (Template && (!bStartingSector || Template->bCanBeStartingSector)) Eligible.AddUnique(Template);
	}
	if (Eligible.IsEmpty()) return nullptr;
	Eligible.Sort([](const UPlanetSectorTemplate& A, const UPlanetSectorTemplate& B)
	{
		return A.GetPathName() < B.GetPathName();
	});
	const uint32 SelectionSeed = static_cast<uint32>(WorldSeed)
		^ (static_cast<uint32>(SectorId) * 7919u) ^ 0x53454354u;
	FRandomStream Random(static_cast<int32>(SelectionSeed & 0x7fffffffu));
	return Eligible[Random.RandRange(0, Eligible.Num() - 1)];
}
