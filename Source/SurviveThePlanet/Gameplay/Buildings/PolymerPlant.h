#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "PolymerPlant.generated.h"

class UPointLightComponent;
class UResourceProductionComponent;
class UStaticMeshComponent;

/** Compact chemical plant using shared construction, placement and electricity rules. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API APolymerPlant : public ABaseBuilding
{
	GENERATED_BODY()
public:
	APolymerPlant();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetPlacementPreview(bool bPreview) override;
	virtual void SetPlacementPreviewValid(bool bValidPlacement) override;
	UFUNCTION(BlueprintPure, Category = "Polymer Production")
	bool IsProducing() const { return bIsProducing; }
	UFUNCTION(BlueprintPure, Category = "Polymer Production")
	float GetPolymerProductionPerMinute() const;
	UFUNCTION(BlueprintPure, Category = "Polymer Production")
	float GetCoalConsumptionPerMinute() const;
	UFUNCTION(BlueprintPure, Category = "Polymer Production")
	float GetWaterConsumptionPerMinute() const;
protected:
	virtual void BeginPlay() override;
	UPROPERTY(EditDefaultsOnly, Category = "Polymer Visuals")
	TObjectPtr<UStaticMesh> FanMeshAsset;
	UPROPERTY(EditDefaultsOnly, Category = "Polymer Visuals")
	TObjectPtr<UStaticMesh> ProductMeshAsset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Polymer Visuals")
	TObjectPtr<UStaticMeshComponent> FanA;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Polymer Visuals")
	TObjectPtr<UStaticMeshComponent> FanB;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Polymer Visuals")
	TObjectPtr<UStaticMeshComponent> OutputBlock;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Polymer Visuals")
	TObjectPtr<UPointLightComponent> ProcessLight;
private:
	UPROPERTY(Transient)
	TObjectPtr<UResourceProductionComponent> Production;
	bool bIsProducing = false;
	float VisualTimeSeconds = 0.0f;
	const UPolymerPlantBuildingDataAsset* GetRecipe() const;
	void RefreshVisuals();
};

