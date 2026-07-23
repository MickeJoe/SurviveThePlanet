#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "EnergyModule.generated.h"

class UStaticMesh;

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AEnergyModule : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AEnergyModule();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Energy Module")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintCallable, Category = "Energy Module")
	void SetPlacementPreviewValid(bool bValidPlacement);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Energy Module")
	TObjectPtr<UStaticMesh> ModuleMesh;

private:
	bool bPlacementPreview = false;
	bool bPlacementPreviewValid = true;

	void ApplyModuleMesh();
	void RefreshPlacementPreviewVisual();
};