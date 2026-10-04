#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EnergyCoverageComponent.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Presentation only: never participates in power, collision, or placement rules. */
UCLASS(ClassGroup=(Energy), meta=(BlueprintSpawnableComponent))
class SURVIVETHEPLANET_API UEnergyCoverageComponent : public USceneComponent
{
	GENERATED_BODY()
public:
	UEnergyCoverageComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage", meta=(ClampMin="0", Units="cm"))
	float CoverageRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance")
	TObjectPtr<UMaterialInterface> CoverageMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance")
	FLinearColor CoverageColor = FLinearColor(0.02f, 0.55f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance", meta=(ClampMin="0", ClampMax="1"))
	float InteriorOpacity = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance", meta=(ClampMin="0", ClampMax="1"))
	float BoundaryOpacity = 0.45f;

	/** Width as a fraction of the coverage radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance", meta=(ClampMin="0.001", ClampMax="1"))
	float BoundaryWidth = 0.018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance", meta=(ClampMin="0.001", ClampMax="1"))
	float EdgeSoftness = 0.008f;

	/** Half-depth of the downward projection, allowing uneven ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Energy Coverage|Appearance", meta=(ClampMin="1", Units="cm"))
	float ProjectionDepth = 2000.0f;

	UFUNCTION(BlueprintCallable, Category="Energy Coverage")
	void SetCoverageVisualizationVisible(bool bShowCoverage);

	UFUNCTION(BlueprintCallable, Category="Energy Coverage")
	void RefreshCoverageVisualization();

	/** Reapply the placement visibility after construction state changes. */
	void RefreshCoverageEligibility();

	bool IsEligibleSource() const;

	UFUNCTION(BlueprintPure, Category="Energy Coverage")
	bool IsCoverageVisualizationVisible() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> CoverageDecal;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoverageMaterialInstance;
};
