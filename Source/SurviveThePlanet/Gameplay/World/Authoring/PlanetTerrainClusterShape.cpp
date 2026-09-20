#include "PlanetTerrainClusterShape.h"

#if WITH_EDITOR
#include "PlanetTerrainClusterShapeActor.h"
#include "UObject/UObjectIterator.h"

void UPlanetTerrainClusterShape::ApplySelectedPreset()
{
	Modify();

	switch (Preset)
	{
	case EPlanetTerrainClusterShapePreset::Shape1_Tapered:
		ShapeId = TEXT("Shape_1_Tapered");
		Points = {{-420, -330}, {-380, 20}, {-250, 170}, {-70, 280}, {100, 220}, {70, 40}, {10, -230}};
		break;
	case EPlanetTerrainClusterShapePreset::Shape2_Curved:
		ShapeId = TEXT("Shape_2_Curved");
		Points = {{-520, -130}, {-390, 10}, {-230, 70}, {-80, 240}, {120, 320}, {390, 310}, {500, 210}, {350, 80}, {80, 100}, {-80, -30}, {-250, -100}};
		break;
	case EPlanetTerrainClusterShapePreset::Shape3_Long:
		ShapeId = TEXT("Shape_3_Long");
		Points = {{-600, -150}, {-500, 100}, {-300, 220}, {250, 200}, {550, 80}, {650, -120}, {400, -250}, {-350, -270}};
		break;
	case EPlanetTerrainClusterShapePreset::Shape4_Slender:
		ShapeId = TEXT("Shape_4_Slender");
		Points = {{-520, -100}, {-350, 20}, {-170, 80}, {20, 190}, {280, 220}, {520, 110}, {390, -20}, {140, -90}, {-100, -170}, {-350, -190}};
		break;
	case EPlanetTerrainClusterShapePreset::Shape5_Forked:
		ShapeId = TEXT("Shape_5_Forked");
		Points = {{-450, 330}, {-260, 250}, {-100, 30}, {0, -140}, {60, -350}, {250, -260}, {330, -80}, {250, 100}, {100, 210}, {-40, 420}, {-230, 470}};
		break;
	case EPlanetTerrainClusterShapePreset::Shape6_SmallBlob:
		ShapeId = TEXT("Shape_6_SmallBlob");
		Points = {{-300, -80}, {-210, 150}, {-40, 260}, {180, 220}, {310, 40}, {250, -170}, {40, -280}, {-180, -240}};
		break;
	default:
		break;
	}

	MarkPackageDirty();
	PostEditChange();
}

void UPlanetTerrainClusterShape::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (TObjectIterator<APlanetTerrainClusterShapeActor> It; It; ++It)
	{
		if (It->GetShape() == this)
		{
			It->RebuildEditorVisualization();
		}
	}
}
#endif
