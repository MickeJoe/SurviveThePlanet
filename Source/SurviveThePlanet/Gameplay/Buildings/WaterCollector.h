#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "WaterCollector.generated.h"

class APlanetWeatherManager;

/** Collects water at a rate determined by the planet's current rainfall. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AWaterCollector : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AWaterCollector();
	virtual void Tick(float DeltaSeconds) override;

	/** Water produced each minute for every millimetre of hourly precipitation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Water Collector|Production", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WaterPerMinutePerMmOfRain = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Water Collector|Production")
	float GetCurrentRainfallMmPerHour() const;

	UFUNCTION(BlueprintPure, Category = "Water Collector|Production")
	float GetCurrentWaterProductionPerMinute() const;

	UFUNCTION(BlueprintCallable, Category = "Water Collector|Placement")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintCallable, Category = "Water Collector|Placement")
	void SetPlacementPreviewValid(bool bValidPlacement);

	/** Consumers only draw power while rain allows this collector to produce. */
	virtual float GetEnergyConsumptionPerMinute() const override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<APlanetWeatherManager> WeatherManager;

	float PendingWaterOutput = 0.0f;
	bool bPlacementPreview = false;
	bool bPlacementPreviewValid = true;

	void ResolveWeatherManager();
	void RefreshPlacementPreviewVisual();
	const UWaterCollectorBuildingDataAsset* GetWaterCollectorBuildingData() const;
	float GetWaterPerMinutePerMmOfRain() const;
};
