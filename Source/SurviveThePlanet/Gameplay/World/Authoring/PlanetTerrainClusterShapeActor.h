#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetTerrainClusterShapeActor.generated.h"

class APlanetSectorTemplateActor;
class UPlanetTerrainClusterShape;
class USceneComponent;
class USplineComponent;
class UTextRenderComponent;

/** Transformable instance of a reusable terrain-cluster footprint. */
UCLASS()
class SURVIVETHEPLANET_API APlanetTerrainClusterShapeActor : public AActor
{
	GENERATED_BODY()

public:
	APlanetTerrainClusterShapeActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Cluster")
	TObjectPtr<UPlanetTerrainClusterShape> Shape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Cluster")
	TObjectPtr<APlanetSectorTemplateActor> OwnerSector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Cluster")
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Terrain Cluster|Visualization")
	FLinearColor OutlineColor = FLinearColor(0.15f, 0.75f, 1.0f, 1.0f);

	UPlanetTerrainClusterShape* GetShape() const { return Shape; }

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Terrain Cluster|Authoring")
	void ValidateShapeActor();

	void RebuildEditorVisualization();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Terrain Cluster")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Outline;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Label;
#endif
};
