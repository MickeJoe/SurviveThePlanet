#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "ConcretePlant.generated.h"

/** Converts water and stone into concrete while supplied with electricity. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AConcretePlant : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AConcretePlant();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Concrete Plant|Placement")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintCallable, Category = "Concrete Plant|Placement")
	void SetPlacementPreviewValid(bool bValidPlacement);

	UFUNCTION(BlueprintPure, Category = "Concrete Plant|Production")
	float GetConcreteProductionPerMinute() const;

private:
	float CycleProgressSeconds = 0.0f;
	bool bPlacementPreview = false;
	bool bPlacementPreviewValid = true;

	const UConcretePlantBuildingDataAsset* GetConcretePlantData() const;
	void RefreshPlacementPreviewVisual();
};
