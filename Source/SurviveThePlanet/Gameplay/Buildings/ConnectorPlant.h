#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "ConnectorPlant.generated.h"

class UResourceProductionComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AConnectorPlant : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AConnectorPlant();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetPlacementPreview(bool bPreview) override;
	virtual void SetPlacementPreviewValid(bool bValidPlacement) override;

	UFUNCTION(BlueprintPure, Category = "Connector Production")
	bool IsProducing() const { return bIsProducing; }

	UFUNCTION(BlueprintPure, Category = "Connector Production")
	float GetConnectorProductionPerMinute() const;
	UFUNCTION(BlueprintPure, Category = "Connector Production")
	float GetCopperConsumptionPerMinute() const;
	UFUNCTION(BlueprintPure, Category = "Connector Production")
	float GetPolymerConsumptionPerMinute() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Connector Production")
	TObjectPtr<UResourceProductionComponent> Production;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connector Visuals")
	TObjectPtr<UPointLightComponent> ProcessLight;

	UPROPERTY(EditDefaultsOnly, Category = "Connector Visuals")
	TObjectPtr<UStaticMesh> FanMeshA;
	UPROPERTY(EditDefaultsOnly, Category = "Connector Visuals")
	TObjectPtr<UStaticMesh> FanMeshB;
	UPROPERTY(EditDefaultsOnly, Category = "Connector Visuals")
	TObjectPtr<UStaticMesh> ProductMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Connector Visuals")
	TObjectPtr<UMaterialInterface> SparkMaterial;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connector Visuals")
	TObjectPtr<UStaticMeshComponent> FanA;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connector Visuals")
	TObjectPtr<UStaticMeshComponent> FanB;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connector Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> ConveyorProducts;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connector Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> WeldingSparks;

private:
	float VisualTimeSeconds = 0.0f;
	void RefreshVisuals();
	bool bIsProducing = false;
	const UConnectorPlantBuildingDataAsset* GetRecipe() const;
};
