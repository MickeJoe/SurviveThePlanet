#include "Gameplay/Buildings/MiningMachine.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/Resources/BaseResourceSource.h"

AMiningMachine::AMiningMachine()
{
	BuildingTag = TEXT("MiningMachine");
	BuildingType = ESTPBuildingType::MiningMachine;
	DroneWorkType = ESTPDroneWorkType::Mining;
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

	// Imported resource meshes do not necessarily have their pivot at the visual
	// center. Match the center of both mesh footprints and their lowest Z point,
	// so replacing a deposit remains correct regardless of either asset's pivot.
	const UStaticMeshComponent* SourceMeshComponent = CandidateSource->GetResourceMeshComponent();
	if (!SourceMeshComponent || !SourceMeshComponent->GetStaticMesh()
		|| !BuildingMesh || !BuildingMesh->GetStaticMesh())
	{
		return SourceTransformOffset * CandidateSource->GetActorTransform();
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

	return SourceTransformOffset * AlignedTransform;
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
