#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlanetTerrainClusterShape.generated.h"

UENUM(BlueprintType)
enum class EPlanetTerrainClusterShapePreset : uint8
{
	Shape1_Tapered,
	Shape2_Curved,
	Shape3_Long,
	Shape4_Slender,
	Shape5_Forked,
	Shape6_SmallBlob
};

/** Low-detail 2D footprint used only to author terrain-cluster layouts. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetTerrainClusterShape : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Cluster")
	FName ShapeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Cluster", meta = (TitleProperty = "Point"))
	TArray<FVector2D> Points;

#if WITH_EDITORONLY_DATA
	/** Convenience presets based loosely on the six footprints in the initial sector sketch. */
	UPROPERTY(EditAnywhere, Category = "Terrain Cluster|Authoring")
	EPlanetTerrainClusterShapePreset Preset = EPlanetTerrainClusterShapePreset::Shape1_Tapered;
#endif

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Terrain Cluster|Authoring")
	void ApplySelectedPreset();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
