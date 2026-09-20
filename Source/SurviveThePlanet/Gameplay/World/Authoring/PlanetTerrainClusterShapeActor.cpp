#include "PlanetTerrainClusterShapeActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"
#include "PlanetTerrainClusterShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogTerrainClusterAuthoring, Log, All);

APlanetTerrainClusterShapeActor::APlanetTerrainClusterShapeActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	Outline = CreateEditorOnlyDefaultSubobject<USplineComponent>(TEXT("ShapeOutline"));
	if (Outline)
	{
		Outline->SetupAttachment(SceneRoot);
		Outline->SetClosedLoop(true);
		Outline->SetDrawDebug(true);
		Outline->SetHiddenInGame(true);
	}

	Label = CreateEditorOnlyDefaultSubobject<UTextRenderComponent>(TEXT("ShapeLabel"));
	if (Label)
	{
		Label->SetupAttachment(SceneRoot);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextCenter);
		Label->SetWorldSize(55.0f);
		Label->SetHiddenInGame(true);
	}
#endif

	bIsEditorOnlyActor = true;
}

void APlanetTerrainClusterShapeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	RebuildEditorVisualization();
#endif
}

#if WITH_EDITOR
void APlanetTerrainClusterShapeActor::ValidateShapeActor()
{
	if (!Shape)
	{
		UE_LOG(LogTerrainClusterAuthoring, Warning, TEXT("%s has no Shape assigned."), *GetActorLabel());
	}
	else if (Shape->Points.Num() < 3)
	{
		UE_LOG(LogTerrainClusterAuthoring, Warning, TEXT("%s uses %s, which has fewer than three points."), *GetActorLabel(), *Shape->GetName());
	}
	else
	{
		UE_LOG(LogTerrainClusterAuthoring, Display, TEXT("%s is valid."), *GetActorLabel());
	}

	if (!OwnerSector)
	{
		UE_LOG(LogTerrainClusterAuthoring, Warning, TEXT("%s is not associated with an OwnerSector and will not be baked."), *GetActorLabel());
	}
}

void APlanetTerrainClusterShapeActor::RebuildEditorVisualization()
{
#if WITH_EDITORONLY_DATA
	if (!Outline || !Label)
	{
		return;
	}

	Outline->ClearSplinePoints(false);
	if (Shape)
	{
		for (const FVector2D& Point : Shape->Points)
		{
			Outline->AddSplinePoint(FVector(Point.X, Point.Y, 5.0), ESplineCoordinateSpace::Local, false);
		}
		for (int32 PointIndex = 0; PointIndex < Shape->Points.Num(); ++PointIndex)
		{
			Outline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
		}
	}
	Outline->SetClosedLoop(Shape && Shape->Points.Num() >= 3, false);
	Outline->SetUnselectedSplineSegmentColor(OutlineColor);
	Outline->SetSelectedSplineSegmentColor(OutlineColor * 1.35f);
	Outline->UpdateSpline();

	const FName DisplayName = !SlotId.IsNone() ? SlotId : (Shape ? Shape->ShapeId : TEXT("Missing Shape"));
	Label->SetText(FText::FromName(DisplayName));
	Label->SetTextRenderColor(OutlineColor.ToFColor(true));
	Label->SetRelativeLocation(FVector(0.0, 0.0, 30.0));
	Label->SetRelativeRotation(FRotator(90.0, 0.0, 0.0));
#endif
}

void APlanetTerrainClusterShapeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildEditorVisualization();
}

void APlanetTerrainClusterShapeActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	RebuildEditorVisualization();
}
#endif
