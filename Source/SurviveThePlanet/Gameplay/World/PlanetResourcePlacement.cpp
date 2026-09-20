#include "Gameplay/World/PlanetResourcePlacement.h"

#include "Gameplay/World/HexSectorGrid.h"

namespace
{
	constexpr int32 MaximumRuleCount = 10000;
	const FName AnySlotType = TEXT("Any");

	struct FResourcePlacementCandidate
	{
		int32 SectorId = INDEX_NONE;
		int32 SlotIndex = INDEX_NONE;
		FVector Location = FVector::ZeroVector;
		float Radius = 0.0f;
		FName SlotType;
		TArray<FName> Tags;
		bool bProtected = false;
	};

	float DistanceToSegment(const FVector2D& Point, const FVector2D& SegmentStart, const FVector2D& SegmentEnd)
	{
		const FVector2D Segment = SegmentEnd - SegmentStart;
		const float SegmentLengthSquared = Segment.SizeSquared();
		if (SegmentLengthSquared <= SMALL_NUMBER)
		{
			return FVector2D::Distance(Point, SegmentStart);
		}

		const float Projection = FVector2D::DotProduct(Point - SegmentStart, Segment) / SegmentLengthSquared;
		const float SegmentPosition = FMath::Clamp(Projection, 0.0f, 1.0f);
		return FVector2D::Distance(Point, SegmentStart + Segment * SegmentPosition);
	}

	bool HasValidRanges(const FPlanetResourceRule& Rule)
	{
		const bool bCountsAreValid = Rule.MinCount >= 0
			&& Rule.MaxCount >= Rule.MinCount
			&& Rule.MaxCount <= MaximumRuleCount;
		const bool bQuantitiesAreValid = Rule.MinQuantity >= 1
			&& Rule.MaxQuantity >= Rule.MinQuantity;
		const bool bRadiusIsValid = FMath::IsFinite(Rule.Radius) && Rule.Radius > 0.0f;
		const bool bSpacingIsValid = FMath::IsFinite(Rule.MinimumSpacing) && Rule.MinimumSpacing >= 0.0f;
		const bool bDistanceRangeIsValid = FMath::IsFinite(Rule.MinDistanceFromHQ)
			&& Rule.MinDistanceFromHQ >= 0.0f
			&& FMath::IsFinite(Rule.MaxDistanceFromHQ)
			&& Rule.MaxDistanceFromHQ >= Rule.MinDistanceFromHQ;

		return bCountsAreValid
			&& bQuantitiesAreValid
			&& bRadiusIsValid
			&& bSpacingIsValid
			&& bDistanceRangeIsValid;
	}

	void ValidateRules(const UPlanetResourceDistribution& Distribution, FPlanetResourcePlacementResult& Result)
	{
		TSet<FName> RuleIds;

		for (const FPlanetResourceRule& Rule : Distribution.Rules)
		{
			const bool bIdIsValid = !Rule.Id.IsNone() && !RuleIds.Contains(Rule.Id);
			if (!bIdIsValid || !HasValidRanges(Rule))
			{
				Result.Errors.Add(FString::Printf(
					TEXT("Rule '%s': IDs must be unique; counts, quantities, radius, spacing and HQ distance ranges must be valid."),
					*Rule.Id.ToString()));
			}

			RuleIds.Add(Rule.Id);
		}
	}

	bool OverlapsBuildablePocket(const FSectorResourceSlot& Slot, const FSectorBuildablePocket& Pocket)
	{
		const FVector2D DistanceOutsidePocket = (Slot.Location - Pocket.Center).GetAbs() - Pocket.Extent;
		const FVector2D ClampedDistance(
			FMath::Max(0.0, DistanceOutsidePocket.X),
			FMath::Max(0.0, DistanceOutsidePocket.Y));

		return ClampedDistance.Size() < Slot.Radius;
	}

	bool OverlapsDroneCorridor(const FSectorResourceSlot& Slot, const FSectorCorridor& Corridor)
	{
		const float RequiredClearance = Slot.Radius + Corridor.Width * 0.5f;
		return DistanceToSegment(Slot.Location, Corridor.Start, Corridor.End) < RequiredClearance;
	}

	bool IsProtectedSlot(const FSectorResourceSlot& Slot, const FSectorTemplateDefinition& SectorTemplate)
	{
		if (!FMath::IsFinite(Slot.Radius) || Slot.Radius <= 0.0f || Slot.Location.ContainsNaN())
		{
			return true;
		}

		for (const FSectorBuildablePocket& Pocket : SectorTemplate.BuildablePockets)
		{
			if (OverlapsBuildablePocket(Slot, Pocket))
			{
				return true;
			}
		}

		for (const FSectorCorridor& Corridor : SectorTemplate.DroneCorridors)
		{
			if (OverlapsDroneCorridor(Slot, Corridor))
			{
				return true;
			}
		}

		return false;
	}

	FVector GetSlotWorldLocation(
		const AHexSectorGrid& Grid,
		const FHexSector& Sector,
		const FSectorResourceSlot& Slot)
	{
		FVector2D LocalLocation = Slot.Location;
		if (Sector.bTemplateMirrored)
		{
			LocalLocation.X *= -1.0f;
		}

		LocalLocation = LocalLocation.GetRotated(Sector.TemplateRotationDegrees);
		const FVector WorldOffset = Grid.GetActorTransform().TransformVectorNoScale(FVector(LocalLocation, 0.0f));
		return Sector.WorldCenter + WorldOffset;
	}

	TArray<FResourcePlacementCandidate> BuildCandidates(
		const AHexSectorGrid& Grid,
		FPlanetResourcePlacementResult& Result)
	{
		TArray<FResourcePlacementCandidate> Candidates;

		for (const FHexSector& Sector : Grid.Sectors)
		{
			FSectorTemplateDefinition SectorTemplate;
			if (!Grid.GetTemplateForSector(Sector.Id, SectorTemplate))
			{
				Result.Errors.Add(FString::Printf(
					TEXT("Sector %d has missing template '%s'."),
					Sector.Id,
					*Sector.TemplateId.ToString()));
				continue;
			}

			for (int32 SlotIndex = 0; SlotIndex < SectorTemplate.ResourceSlots.Num(); ++SlotIndex)
			{
				const FSectorResourceSlot& Slot = SectorTemplate.ResourceSlots[SlotIndex];

				FResourcePlacementCandidate Candidate;
				Candidate.SectorId = Sector.Id;
				Candidate.SlotIndex = SlotIndex;
				Candidate.Location = GetSlotWorldLocation(Grid, Sector, Slot);
				Candidate.Radius = Slot.Radius;
				Candidate.SlotType = Slot.SlotType;
				Candidate.Tags = SectorTemplate.DressingRuleTags;
				Candidate.bProtected = IsProtectedSlot(Slot, SectorTemplate);
				Candidates.Add(MoveTemp(Candidate));
			}
		}

		Candidates.Sort([](const FResourcePlacementCandidate& Left, const FResourcePlacementCandidate& Right)
		{
			return Left.SectorId == Right.SectorId
				? Left.SlotIndex < Right.SlotIndex
				: Left.SectorId < Right.SectorId;
		});

		return Candidates;
	}

	TArray<int32> BuildRuleOrder(const UPlanetResourceDistribution& Distribution)
	{
		TArray<int32> RuleOrder;
		RuleOrder.Reserve(Distribution.Rules.Num());

		for (int32 RuleIndex = 0; RuleIndex < Distribution.Rules.Num(); ++RuleIndex)
		{
			RuleOrder.Add(RuleIndex);
		}

		RuleOrder.StableSort([&Distribution](int32 LeftIndex, int32 RightIndex)
		{
			return Distribution.Rules[LeftIndex].bGuaranteed
				&& !Distribution.Rules[RightIndex].bGuaranteed;
		});

		return RuleOrder;
	}

	TArray<int32> BuildShuffledCandidateOrder(int32 CandidateCount, FRandomStream& Random)
	{
		TArray<int32> CandidateOrder;
		CandidateOrder.Reserve(CandidateCount);

		for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
		{
			CandidateOrder.Add(CandidateIndex);
		}

		for (int32 Index = CandidateOrder.Num() - 1; Index > 0; --Index)
		{
			CandidateOrder.Swap(Index, Random.RandRange(0, Index));
		}

		return CandidateOrder;
	}

	FString GetCandidateRejectionReason(
		const FResourcePlacementCandidate& Candidate,
		const FPlanetResourceRule& Rule,
		const FVector& HeadquartersLocation,
		const TArray<FPlanetPlacedDeposit>& ExistingDeposits)
	{
		if (Candidate.bProtected)
		{
			return TEXT("Protected pocket/corridor or invalid slot geometry");
		}

		if (!Rule.AllowedSectorIds.IsEmpty() && !Rule.AllowedSectorIds.Contains(Candidate.SectorId))
		{
			return TEXT("Outside selected sectors");
		}

		if (Candidate.SlotType != AnySlotType && Rule.SlotType != AnySlotType && Candidate.SlotType != Rule.SlotType)
		{
			return TEXT("Slot type mismatch");
		}

		if (Rule.Radius > Candidate.Radius)
		{
			return TEXT("Deposit exceeds slot radius");
		}

		const float DistanceFromHeadquarters = FVector::Dist2D(Candidate.Location, HeadquartersLocation);
		if (DistanceFromHeadquarters < Rule.MinDistanceFromHQ || DistanceFromHeadquarters > Rule.MaxDistanceFromHQ)
		{
			return TEXT("Outside HQ distance band");
		}

		for (const FName& RequiredTag : Rule.RequiredTemplateTags)
		{
			if (!Candidate.Tags.Contains(RequiredTag))
			{
				return TEXT("Missing template tag");
			}
		}

		for (const FPlanetPlacedDeposit& ExistingDeposit : ExistingDeposits)
		{
			if (ExistingDeposit.SectorId == Candidate.SectorId && ExistingDeposit.SlotIndex == Candidate.SlotIndex)
			{
				return TEXT("Occupied");
			}

			const float RequiredSpacing = Rule.Radius
				+ ExistingDeposit.Radius
				+ FMath::Max(Rule.MinimumSpacing, ExistingDeposit.MinimumSpacing);
			if (FVector::Dist2D(ExistingDeposit.Location, Candidate.Location) < RequiredSpacing)
			{
				return TEXT("Minimum spacing conflict");
			}
		}

		return FString();
	}

	FPlanetPlacedDeposit CreateDeposit(
		const FResourcePlacementCandidate& Candidate,
		const FPlanetResourceRule& Rule,
		FRandomStream& Random)
	{
		FPlanetPlacedDeposit Deposit;
		Deposit.RuleId = Rule.Id;
		Deposit.ResourceType = Rule.ResourceType;
		Deposit.SectorId = Candidate.SectorId;
		Deposit.SlotIndex = Candidate.SlotIndex;
		Deposit.Location = Candidate.Location;
		Deposit.Quantity = Random.RandRange(Rule.MinQuantity, Rule.MaxQuantity);
		Deposit.Radius = Rule.Radius;
		Deposit.MinimumSpacing = Rule.MinimumSpacing;
		return Deposit;
	}

	void AddDiagnostic(
		const FResourcePlacementCandidate& Candidate,
		const FPlanetResourceRule& Rule,
		const FString& Reason,
		FPlanetResourcePlacementResult& Result)
	{
		FPlanetSlotDiagnostic Diagnostic;
		Diagnostic.RuleId = Rule.Id;
		Diagnostic.SectorId = Candidate.SectorId;
		Diagnostic.SlotIndex = Candidate.SlotIndex;
		Diagnostic.Location = Candidate.Location;
		Diagnostic.Reason = Reason.IsEmpty() ? TEXT("Compatible") : Reason;
		Result.Slots.Add(MoveTemp(Diagnostic));
	}

	void PlaceRule(
		const FPlanetResourceRule& Rule,
		const TArray<FResourcePlacementCandidate>& Candidates,
		const FVector& HeadquartersLocation,
		FRandomStream& Random,
		FPlanetResourcePlacementResult& Result)
	{
		const int32 WantedCount = Random.RandRange(Rule.MinCount, Rule.MaxCount);
		const TArray<int32> CandidateOrder = BuildShuffledCandidateOrder(Candidates.Num(), Random);
		int32 PlacedCount = 0;

		for (const int32 CandidateIndex : CandidateOrder)
		{
			const FResourcePlacementCandidate& Candidate = Candidates[CandidateIndex];
			FString RejectionReason = GetCandidateRejectionReason(
				Candidate,
				Rule,
				HeadquartersLocation,
				Result.Deposits);

			if (RejectionReason.IsEmpty() && PlacedCount < WantedCount)
			{
				Result.Deposits.Add(CreateDeposit(Candidate, Rule, Random));
				++PlacedCount;
				RejectionReason = TEXT("Occupied");
			}

			AddDiagnostic(Candidate, Rule, RejectionReason, Result);
		}

		if (PlacedCount >= WantedCount)
		{
			return;
		}

		const FString Message = FString::Printf(
			TEXT("Rule '%s': placed %d/%d. Inspect Slots for type, size, HQ distance, tag, protected area or spacing rejections; add compatible slots or relax constraints."),
			*Rule.Id.ToString(),
			PlacedCount,
			WantedCount);

		if (Rule.bGuaranteed)
		{
			Result.Errors.Add(Message);
		}
		else
		{
			Result.Warnings.Add(Message);
		}
	}
}

UPlanetResourcePlacementComponent::UPlanetResourcePlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FPlanetResourcePlacementResult UPlanetResourcePlacementComponent::PlaceResources(
	const AHexSectorGrid* Grid, const UPlanetResourceDistribution* Input, int32 Seed)
{
	FPlanetResourcePlacementResult Result;
	Result.PlacementSeed = Seed;

	FHexSector HeadquartersSector;
	if (!Grid || !Input || !Grid->GetSectorById(Grid->StartingSectorId, HeadquartersSector))
	{
		Result.Errors.Add(TEXT("Supply a distribution and an already generated layout with a valid HQ sector."));
		return Result;
	}

	Result.LayoutSeed = Grid->LayoutSeed;
	ValidateRules(*Input, Result);
	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	const TArray<FResourcePlacementCandidate> Candidates = BuildCandidates(*Grid, Result);
	if (!Result.Errors.IsEmpty())
	{
		return Result;
	}

	FRandomStream Random(Seed);
	const TArray<int32> RuleOrder = BuildRuleOrder(*Input);
	for (const int32 RuleIndex : RuleOrder)
	{
		PlaceRule(Input->Rules[RuleIndex], Candidates, HeadquartersSector.WorldCenter, Random, Result);
	}

	Result.bSuccess = Result.Errors.IsEmpty();
	return Result;
}

void UPlanetResourcePlacementComponent::Generate()
{
	Result = PlaceResources(Cast<AHexSectorGrid>(GetOwner()), Distribution, PlacementSeed);
}
