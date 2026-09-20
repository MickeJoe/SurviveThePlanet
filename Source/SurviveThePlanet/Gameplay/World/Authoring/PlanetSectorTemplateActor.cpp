#include "PlanetSectorTemplateActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "PlanetSectorTemplate.h"
#include "PlanetTerrainClusterShape.h"
#include "PlanetTerrainClusterShapeActor.h"
#include "PlanetTerrainClusterVariant.h"

DEFINE_LOG_CATEGORY_STATIC(LogSectorTemplateAuthoring, Log, All);

APlanetSectorTemplateActor::APlanetSectorTemplateActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	Boundary = CreateEditorOnlyDefaultSubobject<USplineComponent>(TEXT("SectorBoundary"));
	// Editor binaries launched with -game do not create editor-only subobjects.
	if (Boundary)
	{
		Boundary->SetupAttachment(SceneRoot);
		Boundary->SetClosedLoop(true);
		Boundary->SetDrawDebug(true);
		Boundary->SetHiddenInGame(true);
	}

	OriginX = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("OriginX"));
	if (OriginX)
	{
		OriginX->SetupAttachment(SceneRoot);
		OriginX->ArrowColor = FColor::Red;
		OriginX->ArrowSize = 2.0f;
		OriginX->SetHiddenInGame(true);
	}

	OriginY = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("OriginY"));
	if (OriginY)
	{
		OriginY->SetupAttachment(SceneRoot);
		OriginY->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
		OriginY->ArrowColor = FColor::Green;
		OriginY->ArrowSize = 2.0f;
		OriginY->SetHiddenInGame(true);
	}

	OriginLabel = CreateEditorOnlyDefaultSubobject<UTextRenderComponent>(TEXT("OriginLabel"));
	if (OriginLabel)
	{
		OriginLabel->SetupAttachment(SceneRoot);
		OriginLabel->SetText(FText::FromString(TEXT("SECTOR ORIGIN")));
		OriginLabel->SetHorizontalAlignment(EHTA_Center);
		OriginLabel->SetWorldSize(45.0f);
		OriginLabel->SetRelativeLocation(FVector(0.0, 0.0, 35.0));
		OriginLabel->SetHiddenInGame(true);
	}
#endif

	bIsEditorOnlyActor = true;
}

void APlanetSectorTemplateActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	RebuildEditorVisualization();
#endif
}

#if WITH_EDITOR
void APlanetSectorTemplateActor::ClearPreview()
{
	if (IsValid(GeneratedPreview) && GeneratedPreview->GetOwner() == this)
	{
		GeneratedPreview->Destroy();
	}
	GeneratedPreview = nullptr;
}

void APlanetSectorTemplateActor::GeneratePreview()
{
	if (!TargetTemplate || !VariantLibrary)
	{
		UE_LOG(LogSectorTemplateAuthoring, Error, TEXT("Assign TargetTemplate and VariantLibrary before generating a preview."));
		return;
	}
	ClearPreview();
	GeneratedPreview = GetWorld()->SpawnActor<APlanetGeneratedSector>();
	if (!GeneratedPreview) return;
	GeneratedPreview->SetOwner(this);
	GeneratedPreview->bIsEditorOnlyActor = true;
	GeneratedPreview->SetActorLabel(TEXT("GENERATED_SectorPreview"));
	GeneratedPreview->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
	GeneratedPreview->SectorTemplate = TargetTemplate;
	GeneratedPreview->VariantLibrary = VariantLibrary;
	GeneratedPreview->Seed = PreviewSeed;
	GeneratedPreview->Generate();
}

void APlanetSectorTemplateActor::RebuildEditorVisualization()
{
#if WITH_EDITORONLY_DATA
	if (!Boundary)
	{
		return;
	}

	Boundary->ClearSplinePoints(false);
	for (int32 PointIndex = 0; PointIndex < 6; ++PointIndex)
	{
		const float AngleRadians = FMath::DegreesToRadians(90.0f + PointIndex * 60.0f);
		Boundary->AddSplinePoint(
			FVector(FMath::Cos(AngleRadians) * SectorRadius, FMath::Sin(AngleRadians) * SectorRadius, 0.0),
			ESplineCoordinateSpace::Local,
			false);
		Boundary->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	Boundary->SetClosedLoop(true, false);
	Boundary->SetUnselectedSplineSegmentColor(FLinearColor::Yellow);
	Boundary->SetSelectedSplineSegmentColor(FLinearColor::White);
	Boundary->UpdateSpline();
#endif
}

void APlanetSectorTemplateActor::BakeSectorTemplate()
{
	if (!TargetTemplate)
	{
		UE_LOG(LogSectorTemplateAuthoring, Error, TEXT("%s cannot bake: TargetTemplate is not assigned."), *GetActorLabel());
		return;
	}

	ValidateSectorTemplate();
	TargetTemplate->Modify();
	TargetTemplate->SectorRadius = SectorRadius;
	TargetTemplate->SectorSize = FVector2D(FMath::Sqrt(3.0f) * SectorRadius, 2.0f * SectorRadius);
	TargetTemplate->ClusterSlots.Reset();

	for (TActorIterator<APlanetTerrainClusterShapeActor> It(GetWorld()); It; ++It)
	{
		APlanetTerrainClusterShapeActor* ShapeActor = *It;
		if (ShapeActor->OwnerSector != this || !ShapeActor->Shape)
		{
			continue;
		}

		FPlanetSectorClusterSlot& Slot = TargetTemplate->ClusterSlots.AddDefaulted_GetRef();
		Slot.Shape = ShapeActor->Shape;
		Slot.Transform = ShapeActor->GetActorTransform().GetRelativeTransform(GetActorTransform());
		Slot.SlotId = ShapeActor->SlotId;
	}

	TargetTemplate->MarkPackageDirty();
	TargetTemplate->PostEditChange();
	UE_LOG(LogSectorTemplateAuthoring, Display, TEXT("Baked %d cluster slots into %s."), TargetTemplate->ClusterSlots.Num(), *TargetTemplate->GetName());
}

void APlanetSectorTemplateActor::ValidateSectorTemplate()
{
	int32 WarningCount = 0;
	for (TActorIterator<APlanetTerrainClusterShapeActor> It(GetWorld()); It; ++It)
	{
		const APlanetTerrainClusterShapeActor* ShapeActor = *It;
		if (ShapeActor->OwnerSector != this)
		{
			continue;
		}

		if (!ShapeActor->Shape)
		{
			++WarningCount;
			UE_LOG(LogSectorTemplateAuthoring, Warning, TEXT("%s has no Shape assigned."), *ShapeActor->GetActorLabel());
			continue;
		}

		if (ShapeActor->Shape->Points.Num() < 3)
		{
			++WarningCount;
			UE_LOG(LogSectorTemplateAuthoring, Warning, TEXT("%s uses %s, which has fewer than three points."), *ShapeActor->GetActorLabel(), *ShapeActor->Shape->GetName());
		}

		for (const FVector2D& Point : ShapeActor->Shape->Points)
		{
			const FVector WorldPoint = ShapeActor->GetActorTransform().TransformPosition(FVector(Point.X, Point.Y, 0.0));
			const FVector SectorPoint = GetActorTransform().InverseTransformPosition(WorldPoint);
			const FVector2D LocalPoint(SectorPoint.X, SectorPoint.Y);
			const float AbsX = FMath::Abs(LocalPoint.X);
			const float AbsY = FMath::Abs(LocalPoint.Y);
			const bool bInsideHex = AbsX <= FMath::Sqrt(3.0f) * SectorRadius * 0.5f
				&& AbsY + AbsX / FMath::Sqrt(3.0f) <= SectorRadius;
			if (!bInsideHex)
			{
				++WarningCount;
				UE_LOG(LogSectorTemplateAuthoring, Warning, TEXT("%s extends outside sector %s."), *ShapeActor->GetActorLabel(), *GetActorLabel());
				break;
			}
		}
	}

	UE_LOG(LogSectorTemplateAuthoring, Display, TEXT("Validation of %s completed with %d warning(s)."), *GetActorLabel(), WarningCount);
}

void APlanetSectorTemplateActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildEditorVisualization();
}

void APlanetSectorTemplateActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	RebuildEditorVisualization();
}
#endif
