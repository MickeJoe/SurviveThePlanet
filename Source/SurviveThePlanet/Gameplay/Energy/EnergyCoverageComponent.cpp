#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UEnergyCoverageComponent::UEnergyCoverageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(
		TEXT("/Game/UI/Materials/M_EnergyCoverage.M_EnergyCoverage"));
	CoverageMaterial = Material.Object;
}

void UEnergyCoverageComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UEnergyCoverageSubsystem* Coverage = GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>())
	{
		Coverage->RegisterSource(this);
	}
}

void UEnergyCoverageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEnergyCoverageSubsystem* Coverage = GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>())
	{
		Coverage->UnregisterSource(this);
	}
	if (CoverageDecal) CoverageDecal->DestroyComponent();
	CoverageDecal = nullptr;
	CoverageMaterialInstance = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UEnergyCoverageComponent::SetCoverageVisualizationVisible(bool bShowCoverage)
{
	// A Blueprint may also use this component on a placeable building.
	bShowCoverage = bShowCoverage && IsEligibleSource();
 if (bShowCoverage)
 {
  const auto* System = GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>();
  bShowCoverage = System && System->IsSourceConnectedToPowerGrid(this);
 }
	if (!bShowCoverage)
	{
		if (CoverageDecal) CoverageDecal->SetVisibility(false);
		return;
	}
	if (!CoverageMaterial) return;
	if (!CoverageDecal)
	{
		CoverageDecal = NewObject<UDecalComponent>(GetOwner(), TEXT("EnergyCoverageDecal"));
		CoverageDecal->SetupAttachment(this);
		CoverageDecal->SetAbsolute(false, true, true);
		CoverageDecal->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
		CoverageDecal->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
		CoverageDecal->SetVisibility(false);
		CoverageDecal->SetFadeScreenSize(0.0f);
		CoverageDecal->RegisterComponent();
	}
	RefreshCoverageVisualization();
	CoverageDecal->SetVisibility(CoverageMaterialInstance != nullptr);
}

void UEnergyCoverageComponent::RefreshCoverageVisualization()
{
	if (!CoverageDecal || !CoverageMaterial) return;
	if (!CoverageMaterialInstance || CoverageMaterialInstance->Parent != CoverageMaterial)
	{
		CoverageMaterialInstance = UMaterialInstanceDynamic::Create(CoverageMaterial, this);
		CoverageDecal->SetDecalMaterial(CoverageMaterialInstance);
	}
	CoverageDecal->DecalSize = FVector(FMath::Max(1.0f, ProjectionDepth),
		FMath::Max(0.0f, CoverageRadius), FMath::Max(0.0f, CoverageRadius));
	CoverageDecal->MarkRenderStateDirty();
	CoverageMaterialInstance->SetVectorParameterValue(TEXT("CoverageColor"), CoverageColor);
	CoverageMaterialInstance->SetScalarParameterValue(TEXT("InteriorOpacity"), InteriorOpacity);
	CoverageMaterialInstance->SetScalarParameterValue(TEXT("BoundaryOpacity"), BoundaryOpacity);
	CoverageMaterialInstance->SetScalarParameterValue(TEXT("BoundaryWidth"), BoundaryWidth);
	CoverageMaterialInstance->SetScalarParameterValue(TEXT("EdgeSoftness"), EdgeSoftness);
}

bool UEnergyCoverageComponent::IsCoverageVisualizationVisible() const
{
	return CoverageDecal && CoverageDecal->IsVisible();
}

void UEnergyCoverageComponent::RefreshCoverageEligibility()
{
	if (GetWorld())
	{
		if (UEnergyCoverageSubsystem* Coverage = GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>())
		{
			Coverage->RefreshSource(this);
		}
	}
}

bool UEnergyCoverageComponent::IsEligibleSource() const
{
	const ABaseBuilding* Building = Cast<ABaseBuilding>(GetOwner());
	return IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed() && CoverageRadius > 0 && !(Building && (Building->IsPlacementPreview() || Building->GetConstructionProgress() < 1));
}
