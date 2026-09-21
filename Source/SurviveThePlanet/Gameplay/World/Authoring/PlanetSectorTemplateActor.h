#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetSectorTemplateActor.generated.h"

class UArrowComponent;
class UPlanetSectorTemplate;
class USceneComponent;
class USplineComponent;
class UTextRenderComponent;
class UPlanetTerrainClusterLibrary;
class APlanetGeneratedSector;

/** Editor reference frame and bake controller for one sector template. */
UCLASS()
class SURVIVETHEPLANET_API APlanetSectorTemplateActor : public AActor
{
	GENERATED_BODY()

public:
	APlanetSectorTemplateActor();

	/** Center-to-corner radius. Defaults to the sectors authored in PlanetLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template", meta = (ClampMin = "100.0", Units = "cm"))
	float SectorRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	TObjectPtr<UPlanetSectorTemplate> TargetTemplate;

	UPROPERTY(EditAnywhere, Category="Sector Template|Preview")
	TObjectPtr<UPlanetTerrainClusterLibrary> VariantLibrary;
	UPROPERTY(EditAnywhere, Category="Sector Template|Preview")
	int32 PreviewSeed = 12345;
	UPROPERTY(VisibleAnywhere, Category="Sector Template|Preview")
	TObjectPtr<APlanetGeneratedSector> GeneratedPreview;

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Sector Template|Preview")
	void GeneratePreview();
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Sector Template|Preview")
	void ClearPreview();
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sector Template|Authoring")
	void BakeSectorTemplate();

	UFUNCTION(CallInEditor, Category = "Sector Template|Authoring")
	void ValidateSectorTemplate();

	void RebuildEditorVisualization();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Sector Template")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Boundary;

	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> OriginX;

	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> OriginY;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> OriginLabel;
#endif
};
