#include "Gameplay/Buildings/MiningMachine.h"

#include "Components/StaticMeshComponent.h"
#include "Gameplay/Resources/BaseResourceSource.h"

AMiningMachine::AMiningMachine()
{
	BuildingTag = TEXT("MiningMachine");
	BuildingType = ESTPBuildingType::MiningMachine;
	SupportedResourceTypes.Add(EResourceType::Iron);
}

void AMiningMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetPreviewResourceSource(nullptr);
	if (IsValid(ResourceSource))
	{
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
		|| !SupportedResourceTypes.Contains(CandidateSource->GetResourceType()))
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

	return SourceTransformOffset * CandidateSource->GetActorTransform();
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
