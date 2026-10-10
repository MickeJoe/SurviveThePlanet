#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetWeatherManager.generated.h"

class UPlanetDefinition;

/** The three global weather values used by gameplay and presentation. */
USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FPlanetWeatherState
{
	GENERATED_BODY()

	/** Millimetres of precipitation per hour. The legacy C++ name is retained for asset compatibility. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Precipitation (mm/h)"))
	float PrecipitationPercent = 0.0f;

	/** Wind speed in metres per second. The legacy C++ name is retained for asset compatibility. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Wind Speed (m/s)"))
	float WindPercent = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float SunPercent = 100.0f;

	static FPlanetWeatherState Lerp(const FPlanetWeatherState& From, const FPlanetWeatherState& To, float Alpha);
	void Clamp();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlanetWeatherChanged, FPlanetWeatherState, NewWeather);

/** Simulates one shared weather state for the entire planet. */
UCLASS()
class SURVIVETHEPLANET_API APlanetWeatherManager : public AActor
{
	GENERATED_BODY()

public:
	APlanetWeatherManager();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Planet Weather")
	FPlanetWeatherState GetCurrentWeather() const { return CurrentWeather; }

	UFUNCTION(BlueprintPure, Category = "Planet Weather")
	UPlanetDefinition* GetPlanetDefinition() const { return PlanetDefinition; }

	/** Uses the HUD's authoritative clock, including pause and speed controls. */
	UFUNCTION(BlueprintCallable, Category = "Planet Weather")
	void UpdatePresentation(double TotalGameMinutes);

	UFUNCTION(BlueprintPure, Category = "Planet Weather")
	FText GetDayPhase(double TotalGameMinutes) const;

	/** Overrides the current state and pauses generated transitions until GenerateNextWeather is called. */
	UFUNCTION(BlueprintCallable, Category = "Planet Weather")
	void SetWeatherImmediately(FPlanetWeatherState NewWeather);

	/** Selects the next deterministic target from the active planet definition. */
	UFUNCTION(BlueprintCallable, Category = "Planet Weather")
	void GenerateNextWeather();

	/** Restarts the deterministic weather sequence using the planet seed. */
	UFUNCTION(BlueprintCallable, Category = "Planet Weather")
	void RestartSimulation();

	UPROPERTY(BlueprintAssignable, Category = "Planet Weather")
	FOnPlanetWeatherChanged OnWeatherChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Weather")
	TObjectPtr<UPlanetDefinition> PlanetDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Weather")
	bool bSimulateWeather = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Planet Weather")
	FPlanetWeatherState CurrentWeather;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Planet Weather")
	FPlanetWeatherState TargetWeather;

private:
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> RainStreaks;
	UPROPERTY(Transient) TObjectPtr<class UDirectionalLightComponent> SunLight;
	UPROPERTY(Transient) TObjectPtr<class USkyLightComponent> SkyLight;
	UPROPERTY(Transient) TObjectPtr<class UExponentialHeightFogComponent> Fog;
	UPROPERTY(Transient) TObjectPtr<class UWindDirectionalSourceComponent> WindSource;
	TArray<FVector> RainPositions;
	float ClearSkySunIntensity = 10.0f;
	float ClearSkyAmbientIntensity = 1.0f;
	float SolarElevation(double TotalGameMinutes) const;
	float WetWeatherProbability() const;
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> WindMotes;
	TArray<FVector> WindPositions;
	void InitializePresentation();
	void UpdateRain(float DeltaSeconds);
	FRandomStream RainRandomStream;
	FRandomStream WeatherRandomStream;
	FPlanetWeatherState TransitionStartWeather;
	float PhaseElapsed = 0.0f;
	bool bTransitioning = false;

	void BroadcastWeather();
	void ApplyWeatherCoherence(FPlanetWeatherState& Weather) const;
};
