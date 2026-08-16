#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlanetDefinition.generated.h"

/** A numeric range used when generating a global weather value. */
USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FWeatherPercentageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Minimum = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Maximum = 100.0f;

	float GetRandomValue(FRandomStream& RandomStream) const;
};

/** Settings for the planet-wide simplified weather simulation. */
USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FPlanetWeatherSettings
{
	GENERATED_BODY()

	/** Rainfall intensity in millimetres per hour. 0-12 is a useful Earth-like range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (DisplayName = "Precipitation (mm/h)"))
	FWeatherPercentageRange PrecipitationRange;

	/** Wind speed in metres per second. 0-25 is a useful Earth-like range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (DisplayName = "Wind Speed (m/s)"))
	FWeatherPercentageRange WindRange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (DisplayName = "Sun (%)"))
	FWeatherPercentageRange SunRange;

	/** Game minutes used to move smoothly from one weather state to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0.1", UIMin = "0.1", DisplayName = "Transition Duration (game minutes)"))
	float TransitionDuration = 30.0f;

	/** Game minutes the generated weather remains stable before selecting a new target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Stable Duration (game minutes)"))
	float StableDuration = 60.0f;

	/** How strongly rain limits direct sunlight. 2 means 25 mm/h caps sun at 50%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SunPenaltyPerMmOfRain = 2.0f;
};

/** Planet-specific tuning for the mission confidence pressure system. */
USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FMissionConfidenceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission Confidence", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float InitialConfidence = 100.0f;

	/** Percentage points lost for each hour shown by the game clock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission Confidence", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Decay Per Game Hour"))
	float DecayPerGameHour = 5.0f;
};

/** Data-driven description of a planet and its global weather characteristics. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Name shown to players and in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	FText DisplayName;

	/** Makes the generated weather sequence reproducible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Weather")
	FPlanetWeatherSettings Weather;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Mission Confidence")
	FMissionConfidenceSettings MissionConfidence;
};
