#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/Trading/MerchantDefinition.h"
#include "Gameplay/World/PlanetResourcePlacement.h"
#include "PlanetDefinition.generated.h"

class ABaseResourceSource;

/** A planet-specific surface deposit, planned once and spawned when its sector is discovered. */
USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FPlanetSurfaceDepositSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	FPlanetResourceRule Deposit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	TSoftClassPtr<ABaseResourceSource> SourceClass;

	/** Probability of a deposit in each non-start sector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SectorFrequency = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	bool bGuaranteeStartingDeposit = true;

	/** Preferred landing-sector position; terrain clearance can move it nearby. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resources")
	FVector StartingOffset = FVector(-950.0f, 850.0f, 0.0f);
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	FText ClimateDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	FLinearColor PlanetColor = FLinearColor(0.1f, 0.5f, 0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	TSoftObjectPtr<UWorld> LandingMap;

	/** Rotation period in game hours; shared by the clock, lighting and selection screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Climate", meta = (ClampMin = "1.0"))
	float DayLengthHours = 24.0f;

	UFUNCTION(BlueprintPure, Category = "Planet|Climate")
	double GetDayLengthMinutes() const;

	UFUNCTION(BlueprintPure, Category = "Planet|Climate")
	float GetSolarElevation(double TotalGameMinutes) const;

	/** Hours with the sun above a given elevation; includes polar day/night. */
	UFUNCTION(BlueprintPure, Category = "Planet|Climate")
	float GetLightHours(float MinimumSolarElevation = 0.0f) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Climate", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float LandingLatitudeDegrees = 35.0f;

	/** Solar declination at landing; controls the local season and daylight length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Climate", meta = (ClampMin = "-45.0", ClampMax = "45.0"))
	float SolarDeclinationDegrees = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Climate")
	float MeanTemperatureCelsius = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Climate", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RainProbability = 0.45f;

	/** Makes the generated weather sequence reproducible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	int32 Seed = 12345;

 /** Probability of one geothermal site in each non-start sector (0-1). */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources", meta=(ClampMin="0.0", ClampMax="1.0"))
 float GeothermalSiteFrequency = 0.0f;

 /** Guarantees a accessible test resource in the landing sector. */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources")
 bool bGuaranteeStartingGeothermalSite = false;

 /** Probability of one uranium deposit in each non-start sector (0-1). */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources", meta=(ClampMin="0.0", ClampMax="1.0"))
 float UraniumDepositFrequency = 0.0f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources")
 bool bGuaranteeStartingUraniumDeposit = false;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources", meta=(ClampMin="1"))
 int32 UraniumDepositMinQuantity = 3000;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources", meta=(ClampMin="1"))
 int32 UraniumDepositMaxQuantity = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planet|Resources")
	TArray<FPlanetSurfaceDepositSettings> SurfaceDeposits;

	/** Credits granted when a new game starts on this planet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Economy", meta = (ClampMin = "0", UIMin = "0"))
	int32 StartingCredits = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Weather")
	FPlanetWeatherSettings Weather;

	/** Visits are timed from the first completed Cargo Bay, in game hours. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Trading")
	FMerchantVisitSchedule MerchantVisits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet|Mission Confidence")
	FMissionConfidenceSettings MissionConfidence;
};
