#include "Gameplay/Buildings/MiningMachine.h"
#include "SurviveThePlanet.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/ResourceManager.h"

AMiningMachine::AMiningMachine()
{
	BuildingTag = TEXT("MiningMachine");
	BuildingType = ESTPBuildingType::MiningMachine;
	DroneWorkType = ESTPDroneWorkType::Mining;
	EnergyStorageCapacity = 0.0f;
	EnergyConsumptionPerMinute = 10.0f;
	SupportedResourceTypes.Add(EResourceType::Iron);
	SupportedResourceTypes.Add(EResourceType::Copper);
	SupportedResourceTypes.Add(EResourceType::Stone);
	SupportedResourceTypes.Add(EResourceType::Coal);
	FSTPResourceOutputRate IronOutput;
	IronOutput.Resource = EResourceType::Iron;
	IronOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(IronOutput);

	FSTPResourceOutputRate CopperOutput;
	CopperOutput.Resource = EResourceType::Copper;
	CopperOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(CopperOutput);

	FSTPResourceOutputRate StoneOutput;
	StoneOutput.Resource = EResourceType::Stone;
	StoneOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(StoneOutput);

	FSTPResourceOutputRate CoalOutput;
	CoalOutput.Resource = EResourceType::Coal;
	CoalOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(CoalOutput);

}

void AMiningMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsProducingResource())
	{
		return;
	}

	const EResourceType OutputResource = ResourceSource->GetResourceType();
	const float OutputPerMinute = GetOutputPerMinuteAt100Percent(OutputResource)
		* GetCombinedDroneEfficiency();
	if (OutputPerMinute <= 0.0f)
	{
		return;
	}

	PendingResourceOutput += OutputPerMinute * DeltaSeconds / 60.0f;
	const int32 RequestedAmount = FMath::FloorToInt(PendingResourceOutput);
	if (RequestedAmount <= 0)
	{
		return;
	}

	PendingResourceOutput -= RequestedAmount;
	const int32 ExtractedAmount = ResourceSource->ExtractResource(RequestedAmount);
	if (ExtractedAmount <= 0)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			It->AddResource(OutputResource, ExtractedAmount);
			break;
		}
	}
}

float AMiningMachine::GetOutputPerMinuteAt100Percent(EResourceType ResourceType) const
{
	for (const FSTPResourceOutputRate& Output : GetOutputRates())
	{
		if (Output.Resource == ResourceType)
		{
			return FMath::Max(0.0f, Output.AmountPerMinutePerDroneAt100Percent);
		}
	}
	return 0.0f;
}

float AMiningMachine::GetCurrentOutputPerMinute() const
{
	return IsProducingResource()
		? GetOutputPerMinuteAt100Percent(ResourceSource->GetResourceType()) * GetCombinedDroneEfficiency()
		: 0.0f;
}

bool AMiningMachine::IsProducingResource() const
{
	return !bPlacementPreview && GetConstructionProgress() >= 1.0f && IsOperational()
		&& IsValid(ResourceSource) && ResourceSource->GetRemainingAmount() > 0
		&& GetOutputPerMinuteAt100Percent(ResourceSource->GetResourceType()) > 0.0f
		&& GetCombinedDroneEfficiency() > 0.0f;
}

float AMiningMachine::GetEnergyConsumptionPerMinute() const
{
	const bool bCanProduce = !bPlacementPreview
		&& GetConstructionProgress() >= 1.0f
		&& IsValid(ResourceSource)
		&& ResourceSource->GetRemainingAmount() > 0
		&& GetOutputPerMinuteAt100Percent(ResourceSource->GetResourceType()) > 0.0f
		&& GetCombinedDroneEfficiency() > 0.0f;

	return bCanProduce ? Super::GetEnergyConsumptionPerMinute() : 0.0f;
}

void AMiningMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetPreviewResourceSource(nullptr);
	if (IsValid(ResourceSource))
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
			{
				It->ReleaseCells(this);
				It->ReserveCells(ResourceSource, ResourceSource->GetGridCell(), ResourceSource->GetGridFootprint());
				break;
			}
		}
		ResourceSource->ReleaseMiningMachine(this);
	}

	ResourceSource = nullptr;
	Super::EndPlay(EndPlayReason);
}

bool AMiningMachine::AttachToResourceSource(ABaseResourceSource* NewResourceSource)
{
	UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG attach actor=%s source=%s actorTransform=%s intendedTransform=%s footprint=%s preview=%d"),
		*GetName(), *GetNameSafe(NewResourceSource), *GetActorTransform().ToString(), *GetPlacementTransformForSource(NewResourceSource).ToString(), *GetGridFootprint().ToString(), bPlacementPreview);
	if (IsValid(NewResourceSource))
	{
		for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
		{
			const FSTPGridPlacement Intended = It->GetPlacementForWorldLocation(GetPlacementTransformForSource(NewResourceSource).GetLocation(), GetGridFootprint());
			const bool bClearance = It->HasBuildingClearance(Intended.OriginCell, GetGridFootprint(), this, true);
			UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG attach_precheck origin=(%d,%d) HasBuildingClearance=%d reservedMachine=%s"), Intended.OriginCell.X, Intended.OriginCell.Y, bClearance, *GetNameSafe(NewResourceSource->GetReservedMiningMachine()));
			break;
		}
	}
	const bool bCanMine = !bPlacementPreview && CanMineResourceSource(NewResourceSource);
	if (!bCanMine)
	{
		UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG attach rejected stage=CanMineResourceSource TryReserveMiningMachine=NOT_REACHED ReleaseCells=NOT_REACHED ReserveCells=NOT_REACHED rollback=NOT_NEEDED"));
		return false;
	}

	if (ResourceSource == NewResourceSource)
	{
		return true;
	}

	const bool bReservedSource = NewResourceSource->TryReserveMiningMachine(this);
	UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG TryReserveMiningMachine=%d source=%s owner=%s"), bReservedSource, *NewResourceSource->GetName(), *GetNameSafe(NewResourceSource->GetReservedMiningMachine()));
	if (!bReservedSource)
	{
		return false;
	}

	// The deposit remains alive while hidden, so transfer its occupied cells to
	// the machine. Connectivity and drone navigation must query the visible
	// building, not the replaced resource actor.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			const FSTPGridCell ResourceOrigin = NewResourceSource->GetGridCell();
			It->LogPlacementDiagnostics(NewResourceSource, ResourceOrigin, NewResourceSource->GetGridFootprint(), TEXT("source before ReleaseCells"));
			It->ReleaseCells(NewResourceSource);
			FSTPGridCell RemainingOrigin;
			const bool bCellsRemain = It->TryGetActorOriginCell(NewResourceSource, RemainingOrigin);
			UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG source after ReleaseCells remaining=%d oldOrigin=(%d,%d) oldFootprint=%s"), bCellsRemain, ResourceOrigin.X, ResourceOrigin.Y, *NewResourceSource->GetGridFootprint().ToString());
			const FSTPGridPlacement MachinePlacement = It->GetPlacementForWorldLocation(
				GetActorLocation(), GetGridFootprint());
			It->LogPlacementDiagnostics(this, MachinePlacement.OriginCell, GetGridFootprint(), TEXT("attach before ReserveCells"));
			if (!It->ReserveCells(this, MachinePlacement.OriginCell, GetGridFootprint()))
			{
				const bool bRollback = It->ReserveCells(NewResourceSource, ResourceOrigin, NewResourceSource->GetGridFootprint());
				UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG rollback cellsRestored=%d"), bRollback);
				NewResourceSource->ReleaseMiningMachine(this);
				UE_LOG(LogSurviveThePlanet, Display, TEXT("STP_MINING_DIAG rollback sourceReleased=%d"), NewResourceSource->GetReservedMiningMachine() == nullptr);
				return false;
			}
			break;
		}
	}

	if (IsValid(ResourceSource))
	{
		ResourceSource->ReleaseMiningMachine(this);
	}

	ResourceSource = NewResourceSource;
	return true;
}

bool AMiningMachine::CanMineResourceSource(const ABaseResourceSource* CandidateSource) const
{
	if (!IsValid(CandidateSource)
		|| CandidateSource->GetRemainingAmount() <= 0
		|| !GetSupportedResourceTypes().Contains(CandidateSource->GetResourceType()))
	{
		return false;
	}

	const AMiningMachine* ReservedMachine = CandidateSource->GetReservedMiningMachine();
	if (IsValid(ReservedMachine))
	{
		return ReservedMachine == this;
	}
	for (TActorIterator<APlanetSurfaceManager> It(CandidateSource->GetWorld()); It; ++It)
	{
		FSTPGridPlacement Placement;
		return CanBuildAtSourceTransform(CandidateSource, CandidateSource->GetActorTransform(), *It, Placement);
	}
	return false;
}

bool AMiningMachine::CanBuildAtSourceTransform(const ABaseResourceSource* CandidateSource, const FTransform& SourceTransform,
	APlanetSurfaceManager* Surface, FSTPGridPlacement& OutPlacement, int32 ExtraTerrainMarginCells) const
{
	if (!IsValid(CandidateSource) || !Surface || !GetSupportedResourceTypes().Contains(CandidateSource->GetResourceType())) return false;
	const FIntPoint Footprint = GetGridFootprint();
	OutPlacement = Surface->GetPlacementForWorldLocation(
		GetPlacementTransformForSourceAtTransform(CandidateSource, SourceTransform).GetLocation(), Footprint);
	OutPlacement.bValid = Surface->CanReserveBuildingCells(const_cast<AMiningMachine*>(this),
		OutPlacement.OriginCell, Footprint, CandidateSource)
		&& (ExtraTerrainMarginCells <= 0 || Surface->HasTerrainClearance(OutPlacement.OriginCell, Footprint,
			Surface->GetBuildingClearanceCells() + ExtraTerrainMarginCells));
	return OutPlacement.bValid;
}

FTransform AMiningMachine::GetPlacementTransformForSource(const ABaseResourceSource* CandidateSource) const
{
	return GetPlacementTransformForSourceAtTransform(CandidateSource,
		IsValid(CandidateSource) ? CandidateSource->GetActorTransform() : GetActorTransform());
}

FTransform AMiningMachine::GetPlacementTransformForSourceAtTransform(const ABaseResourceSource* CandidateSource, const FTransform& SourceTransform) const
{
	if (!IsValid(CandidateSource))
	{
		return GetActorTransform();
	}

	// Imported resource meshes do not necessarily have their pivot at the visual
	// center. Match the center of both mesh footprints and their lowest Z point,
	// so replacing a deposit remains correct regardless of either asset's pivot.
	const UStaticMeshComponent* SourceMeshComponent = CandidateSource->GetResourceMeshComponent();
	if (!SourceMeshComponent || !SourceMeshComponent->GetStaticMesh()
		|| !BuildingMesh || !BuildingMesh->GetStaticMesh())
	{
		return GetSourceTransformOffset() * SourceTransform;
	}

	const FBox SourceBounds = SourceMeshComponent->GetStaticMesh()->GetBoundingBox();
	const FBox MachineBounds = BuildingMesh->GetStaticMesh()->GetBoundingBox();
	const FVector SourceAnchorLocal(SourceBounds.GetCenter().X, SourceBounds.GetCenter().Y, SourceBounds.Min.Z);
	const FVector MachineAnchorMeshLocal(MachineBounds.GetCenter().X, MachineBounds.GetCenter().Y, MachineBounds.Min.Z);
	const FTransform SourceMeshToActor = SourceMeshComponent->GetComponentTransform().GetRelativeTransform(CandidateSource->GetActorTransform());
	const FVector SourceAnchorWorld = (SourceMeshToActor * SourceTransform).TransformPosition(SourceAnchorLocal);
	const FVector MachineAnchorActorLocal = BuildingMesh->GetRelativeTransform().TransformPosition(MachineAnchorMeshLocal);

	FTransform AlignedTransform(
		SourceTransform.GetRotation(),
		SourceTransform.GetLocation(),
		GetActorScale3D());
	const FVector MachineAnchorWorld = AlignedTransform.TransformPosition(MachineAnchorActorLocal);
	AlignedTransform.AddToTranslation(SourceAnchorWorld - MachineAnchorWorld);

	return GetSourceTransformOffset() * AlignedTransform;
}

void AMiningMachine::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	bPlacementPreview = bPreview;
	bIsSelectable = !bPreview;
	SetActorEnableCollision(!bPreview);
	SetConstructionProgress(bPreview ? 1.0f : 0.0f);

	if (!bPreview)
	{
		SetPreviewResourceSource(nullptr);
	}

	if (BuildingMesh)
	{
		BuildingMesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		BuildingMesh->SetRenderCustomDepth(bPreview);
	}

	RefreshPlacementPreviewVisual();
}

void AMiningMachine::SetPlacementPreviewValid(bool bValidPlacement)
{
	Super::SetPlacementPreviewValid(bValidPlacement);
	bPlacementPreviewValid = bValidPlacement;
	RefreshPlacementPreviewVisual();
}

void AMiningMachine::SetPreviewResourceSource(ABaseResourceSource* NewPreviewSource)
{
	if (PreviewResourceSource == NewPreviewSource)
	{
		return;
	}

	if (IsValid(PreviewResourceSource))
	{
		PreviewResourceSource->SetPreviewingMiningMachine(nullptr);
	}

	PreviewResourceSource = NewPreviewSource;
	if (bPlacementPreview && IsValid(PreviewResourceSource))
	{
		PreviewResourceSource->SetPreviewingMiningMachine(this);
	}
}

void AMiningMachine::RefreshPlacementPreviewVisual()
{
	if (BuildingMesh)
	{
		BuildingMesh->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}

const UMiningBuildingDataAsset* AMiningMachine::GetMiningBuildingData() const
{
	return Cast<UMiningBuildingDataAsset>(BuildingData);
}

const TArray<EResourceType>& AMiningMachine::GetSupportedResourceTypes() const
{
	if (const UMiningBuildingDataAsset* MiningData = GetMiningBuildingData())
	{
		return MiningData->SupportedResourceTypes;
	}

	return SupportedResourceTypes;
}

const TArray<FSTPResourceOutputRate>& AMiningMachine::GetOutputRates() const
{
	if (const UMiningBuildingDataAsset* MiningData = GetMiningBuildingData())
	{
		if (!MiningData->OutputPerDroneAt100Percent.IsEmpty())
		{
			return MiningData->OutputPerDroneAt100Percent;
		}
	}
	return OutputPerDroneAt100Percent;
}

FTransform AMiningMachine::GetSourceTransformOffset() const
{
	if (const UMiningBuildingDataAsset* MiningData = GetMiningBuildingData())
	{
		return MiningData->SourceTransformOffset;
	}

	return SourceTransformOffset;
}
