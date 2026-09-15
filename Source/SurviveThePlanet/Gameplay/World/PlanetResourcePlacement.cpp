#include "Gameplay/World/PlanetResourcePlacement.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "DrawDebugHelpers.h"

namespace
{
	struct FCandidate
	{
		int32 SectorId;
		int32 SlotIndex;
		FVector Location;
		float Radius;
		FName Type;
		TArray<FName> Tags;
		bool bProtected;
	};

	float SegmentDistance(FVector2D P, FVector2D A, FVector2D B)
	{
		const FVector2D D = B - A;
		const float T = D.SizeSquared() > SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(P - A, D) / D.SizeSquared(), 0.0f, 1.0f) : 0;
		return (P - (A + D * T)).Size();
	}
}

UPlanetResourcePlacementComponent::UPlanetResourcePlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}

FPlanetResourcePlacementResult UPlanetResourcePlacementComponent::PlaceResources(
	const AHexSectorGrid* Grid, const UPlanetResourceDistribution* Input, int32 Seed)
{
	FPlanetResourcePlacementResult Out;
	Out.PlacementSeed = Seed;
	FHexSector HQ;
	if (!Grid || !Input || !Grid->GetSectorById(Grid->StartingSectorId, HQ))
	{
		Out.Errors.Add(TEXT("Supply a distribution and an already generated layout with a valid HQ sector."));
		return Out;
	}
	Out.LayoutSeed = Grid->LayoutSeed;
	TSet<FName> Ids;
	for (const FPlanetResourceRule& Rule : Input->Rules)
	{
		if (Rule.Id.IsNone() || Ids.Contains(Rule.Id) || Rule.MinCount < 0 || Rule.MaxCount < Rule.MinCount
			|| Rule.MaxCount > 10000 || Rule.MinQuantity < 1 || Rule.MaxQuantity < Rule.MinQuantity
			|| !FMath::IsFinite(Rule.Radius) || Rule.Radius <= 0
			|| !FMath::IsFinite(Rule.MinimumSpacing) || Rule.MinimumSpacing < 0
			|| !FMath::IsFinite(Rule.MinDistanceFromHQ) || Rule.MinDistanceFromHQ < 0
			|| !FMath::IsFinite(Rule.MaxDistanceFromHQ) || Rule.MaxDistanceFromHQ < Rule.MinDistanceFromHQ)
			Out.Errors.Add(FString::Printf(TEXT("Rule '%s': IDs must be unique; counts, quantities, radius, spacing and HQ distance ranges must be valid."), *Rule.Id.ToString()));
		Ids.Add(Rule.Id);
	}
	if (!Out.Errors.IsEmpty()) return Out;

	TArray<FCandidate> Candidates;
	for (const FHexSector& Sector : Grid->Sectors)
	{
		FSectorTemplateDefinition Template;
		if (!Grid->GetTemplateForSector(Sector.Id, Template))
		{
			Out.Errors.Add(FString::Printf(TEXT("Sector %d has missing template '%s'."), Sector.Id, *Sector.TemplateId.ToString()));
			continue;
		}
		for (int32 Index = 0; Index < Template.ResourceSlots.Num(); ++Index)
		{
			const FSectorResourceSlot& Slot = Template.ResourceSlots[Index];
			// Reserve the entire authored slot envelope, keeping buildable pockets and drone corridors clear.
			bool bProtected = !FMath::IsFinite(Slot.Radius) || Slot.Radius <= 0 || Slot.Location.ContainsNaN();
			for (const FSectorBuildablePocket& Pocket : Template.BuildablePockets)
			{
				const FVector2D Delta = (Slot.Location - Pocket.Center).GetAbs() - Pocket.Extent;
				bProtected |= FVector2D(FMath::Max(0.0, Delta.X), FMath::Max(0.0, Delta.Y)).Size() < Slot.Radius;
			}
			for (const FSectorCorridor& Corridor : Template.DroneCorridors)
			{
				bProtected |= SegmentDistance(Slot.Location, Corridor.Start, Corridor.End) < Slot.Radius + Corridor.Width * 0.5f;
			}
			FVector2D Local = Slot.Location;
			if (Sector.bTemplateMirrored) Local.X *= -1;
			Local = Local.GetRotated(Sector.TemplateRotationDegrees);
			Candidates.Add({Sector.Id, Index, Sector.WorldCenter + Grid->GetActorTransform().TransformVectorNoScale(FVector(Local, 0)),
				Slot.Radius, Slot.SlotType, Template.DressingRuleTags, bProtected});
		}
	}
	if (!Out.Errors.IsEmpty()) return Out;
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.SectorId == B.SectorId ? A.SlotIndex < B.SlotIndex : A.SectorId < B.SectorId; });
	FRandomStream Random(Seed);
	TArray<int32> Order;
	for (int32 I = 0; I < Input->Rules.Num(); ++I) Order.Add(I);
	Order.StableSort([&](int32 A, int32 B) { return Input->Rules[A].bGuaranteed && !Input->Rules[B].bGuaranteed; });
	for (int32 RuleIndex : Order)
	{
		const FPlanetResourceRule& Rule = Input->Rules[RuleIndex];
		const int32 Wanted = Random.RandRange(Rule.MinCount, Rule.MaxCount);
		TArray<int32> Shuffled;
		for (int32 I = 0; I < Candidates.Num(); ++I) Shuffled.Add(I);
		for (int32 I = Shuffled.Num() - 1; I > 0; --I) Shuffled.Swap(I, Random.RandRange(0, I));
		int32 Placed = 0;
		for (int32 CandidateIndex : Shuffled)
		{
			const FCandidate& C = Candidates[CandidateIndex];
			FString Reason;
			const float Distance = FVector::Dist2D(C.Location, HQ.WorldCenter);
			if (C.bProtected) Reason = TEXT("Protected pocket/corridor or invalid slot geometry");
			else if (!Rule.AllowedSectorIds.IsEmpty() && !Rule.AllowedSectorIds.Contains(C.SectorId)) Reason = TEXT("Outside selected sectors");
			else if (C.Type != TEXT("Any") && Rule.SlotType != TEXT("Any") && C.Type != Rule.SlotType) Reason = TEXT("Slot type mismatch");
			else if (Rule.Radius > C.Radius) Reason = TEXT("Deposit exceeds slot radius");
			else if (Distance < Rule.MinDistanceFromHQ || Distance > Rule.MaxDistanceFromHQ) Reason = TEXT("Outside HQ distance band");
			for (FName Tag : Rule.RequiredTemplateTags) if (Reason.IsEmpty() && !C.Tags.Contains(Tag)) Reason = TEXT("Missing template tag");
			for (const FPlanetPlacedDeposit& Existing : Out.Deposits)
			{
				if (!Reason.IsEmpty()) break;
				if (Existing.SectorId == C.SectorId && Existing.SlotIndex == C.SlotIndex) Reason = TEXT("Occupied");
				else if (FVector::Dist2D(Existing.Location, C.Location) < Rule.Radius + Existing.Radius + FMath::Max(Rule.MinimumSpacing, Existing.MinimumSpacing)) Reason = TEXT("Minimum spacing conflict");
			}
			if (Reason.IsEmpty() && Placed < Wanted)
			{
				FPlanetPlacedDeposit Deposit;
				Deposit.RuleId = Rule.Id; Deposit.ResourceType = Rule.ResourceType;
				Deposit.SectorId = C.SectorId; Deposit.SlotIndex = C.SlotIndex; Deposit.Location = C.Location;
				Deposit.Radius = Rule.Radius; Deposit.MinimumSpacing = Rule.MinimumSpacing;
				Deposit.Quantity = Random.RandRange(Rule.MinQuantity, Rule.MaxQuantity);
				Out.Deposits.Add(Deposit); ++Placed; Reason = TEXT("Occupied");
			}
			FPlanetSlotDiagnostic Diagnostic;
			Diagnostic.RuleId = Rule.Id; Diagnostic.SectorId = C.SectorId; Diagnostic.SlotIndex = C.SlotIndex;
			Diagnostic.Location = C.Location; Diagnostic.Reason = Reason.IsEmpty() ? TEXT("Compatible") : Reason;
			Out.Slots.Add(Diagnostic);
		}
		if (Placed < Wanted)
		{
			const FString Message = FString::Printf(TEXT("Rule '%s': placed %d/%d. Inspect Slots for type, size, HQ distance, tag, protected area or spacing rejections; add compatible slots or relax constraints."), *Rule.Id.ToString(), Placed, Wanted);
			if (Rule.bGuaranteed) Out.Errors.Add(Message); else Out.Warnings.Add(Message);
		}
	}
	Out.bSuccess = Out.Errors.IsEmpty();
	return Out;
}

void UPlanetResourcePlacementComponent::Generate()
{
	Result = PlaceResources(Cast<AHexSectorGrid>(GetOwner()), Distribution, PlacementSeed);
}

void UPlanetResourcePlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bShowDebug) return;
	const FName Filter = !DebugRuleId.IsNone() ? DebugRuleId : (Distribution && !Distribution->Rules.IsEmpty() ? Distribution->Rules[0].Id : NAME_None);
	for (const FPlanetSlotDiagnostic& Slot : Result.Slots)
	{
		if (Slot.RuleId != Filter) continue;
		const bool bOccupied = Result.Deposits.ContainsByPredicate([&](const FPlanetPlacedDeposit& D) { return D.SectorId == Slot.SectorId && D.SlotIndex == Slot.SlotIndex; });
		const FColor Color = bOccupied ? FColor::Cyan : Slot.Reason == TEXT("Compatible") ? FColor::Green : FColor::Red;
		DrawDebugSphere(GetWorld(), Slot.Location, 100, 12, Color, false, 0, 0, 3);
		DrawDebugString(GetWorld(), Slot.Location + FVector(0,0,130), FString::Printf(TEXT("%s S%d/%d: %s"), *Filter.ToString(), Slot.SectorId, Slot.SlotIndex, bOccupied ? TEXT("Occupied") : *Slot.Reason), nullptr, Color, 0, true);
	}
}
