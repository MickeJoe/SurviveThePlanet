#include "Gameplay/Buildings/MiningMachine.h"

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
	FSTPResourceOutputRate IronOutput;
	IronOutput.Resource = EResourceType::Iron;
	IronOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(IronOutput);

	FSTPResourceOutputRate CopperOutput;
	CopperOutput.Resource = EResourceType::Copper;
	CopperOutput.AmountPerMinutePerDroneAt100Percent = 10.0f;
	OutputPerDroneAt100Percent.Add(CopperOutput);

}

void AMiningMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPlacementPreview || GetConstructionProgress() < 1.0f || !IsOperational()
		|| !IsValid(ResourceSource) || ResourceSource->GetRemainingAmount() <= 0)
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
	return IsValid(ResourceSource) && IsOperational()
		? GetOutputPerMinuteAt100Percent(ResourceSource->GetResourceType()) * GetCombinedDroneEfficiency()
		: 0.0f;
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
	if (bPlacementPreview || !CanMineResourceSource(NewResourceSource))
	{
		return false;
	}

	if (ResourceSource == NewResourceSource)
	{
		return true;
	}

	if (!NewResourceSource->TryReserveMiningMachine(this))
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
			It->ReleaseCells(NewResourceSource);
			const FSTPGridPlacement MachinePlacement = It->GetPlacementForWorldLocation(
				GetActorLocation(), GetGridFootprint());
			if (!It->ReserveCells(this, MachinePlacement.OriginCell, GetGridFootprint()))
			{
				It->ReserveCells(NewResourceSource, ResourceOrigin, NewResourceSource->GetGridFootprint());
				NewResourceSource->ReleaseMiningMachine(this);
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
	return !IsValid(ReservedMachine) || ReservedMachine == this;
}

FTransform AMiningMachine::GetPlacementTransformForSource(const ABaseResourceSource* CandidateSource) const
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
		return GetSourceTransformOffset() * CandidateSource->GetActorTransform();
	}

	const FBox SourceBounds = SourceMeshComponent->GetStaticMesh()->GetBoundingBox();
	const FBox MachineBounds = BuildingMesh->GetStaticMesh()->GetBoundingBox();
	const FVector SourceAnchorLocal(SourceBounds.GetCenter().X, SourceBounds.GetCenter().Y, SourceBounds.Min.Z);
	const FVector MachineAnchorMeshLocal(MachineBounds.GetCenter().X, MachineBounds.GetCenter().Y, MachineBounds.Min.Z);
	const FVector SourceAnchorWorld = SourceMeshComponent->GetComponentTransform().TransformPosition(SourceAnchorLocal);
	const FVector MachineAnchorActorLocal = BuildingMesh->GetRelativeTransform().TransformPosition(MachineAnchorMeshLocal);

	FTransform AlignedTransform(
		CandidateSource->GetActorQuat(),
		CandidateSource->GetActorLocation(),
		GetActorScale3D());
	const FVector MachineAnchorWorld = AlignedTransform.TransformPosition(MachineAnchorActorLocal);
	AlignedTransform.AddToTranslation(SourceAnchorWorld - MachineAnchorWorld);

	return GetSourceTransformOffset() * AlignedTransform;
}

void AMiningMachine::SetPlacementPreview(bool bPreview)
{
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
