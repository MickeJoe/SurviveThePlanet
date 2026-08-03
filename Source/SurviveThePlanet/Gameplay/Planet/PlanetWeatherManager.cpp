#include "PlanetWeatherManager.h"

#include "PlanetDefinition.h"

FPlanetWeatherState FPlanetWeatherState::Lerp(const FPlanetWeatherState& From, const FPlanetWeatherState& To, float Alpha)
{
	FPlanetWeatherState Result;
	Result.PrecipitationPercent = FMath::Lerp(From.PrecipitationPercent, To.PrecipitationPercent, Alpha);
	Result.WindPercent = FMath::Lerp(From.WindPercent, To.WindPercent, Alpha);
	Result.SunPercent = FMath::Lerp(From.SunPercent, To.SunPercent, Alpha);
	Result.Clamp();
	return Result;
}

void FPlanetWeatherState::Clamp()
{
	PrecipitationPercent = FMath::Max(PrecipitationPercent, 0.0f);
	WindPercent = FMath::Max(WindPercent, 0.0f);
	SunPercent = FMath::Clamp(SunPercent, 0.0f, 100.0f);
}

APlanetWeatherManager::APlanetWeatherManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APlanetWeatherManager::BeginPlay()
{
	Super::BeginPlay();
	RestartSimulation();
}

void APlanetWeatherManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bSimulateWeather || !PlanetDefinition)
	{
		return;
	}

	const FPlanetWeatherSettings& Settings = PlanetDefinition->Weather;
	PhaseElapsed += DeltaSeconds;

	if (bTransitioning)
	{
		const float Alpha = FMath::Clamp(PhaseElapsed / FMath::Max(Settings.TransitionDuration, 0.1f), 0.0f, 1.0f);
		CurrentWeather = FPlanetWeatherState::Lerp(TransitionStartWeather, TargetWeather, Alpha);
		BroadcastWeather();

		if (Alpha >= 1.0f)
		{
			bTransitioning = false;
			PhaseElapsed = 0.0f;
		}
	}
	else if (PhaseElapsed >= Settings.StableDuration)
	{
		GenerateNextWeather();
	}
}

void APlanetWeatherManager::SetWeatherImmediately(FPlanetWeatherState NewWeather)
{
	NewWeather.Clamp();
	ApplyWeatherCoherence(NewWeather);
	CurrentWeather = NewWeather;
	TargetWeather = NewWeather;
	TransitionStartWeather = NewWeather;
	PhaseElapsed = 0.0f;
	bTransitioning = false;
	BroadcastWeather();
}

void APlanetWeatherManager::GenerateNextWeather()
{
	if (!PlanetDefinition)
	{
		return;
	}

	const FPlanetWeatherSettings& Settings = PlanetDefinition->Weather;
	TransitionStartWeather = CurrentWeather;
	TargetWeather.PrecipitationPercent = Settings.PrecipitationRange.GetRandomValue(WeatherRandomStream);
	TargetWeather.WindPercent = Settings.WindRange.GetRandomValue(WeatherRandomStream);
	TargetWeather.SunPercent = Settings.SunRange.GetRandomValue(WeatherRandomStream);
	TargetWeather.Clamp();
	ApplyWeatherCoherence(TargetWeather);
	PhaseElapsed = 0.0f;
	bTransitioning = true;
}

void APlanetWeatherManager::RestartSimulation()
{
	if (!PlanetDefinition)
	{
		SetActorTickEnabled(false);
		return;
	}

	SetActorTickEnabled(true);
	WeatherRandomStream.Initialize(PlanetDefinition->Seed);
	CurrentWeather.PrecipitationPercent = PlanetDefinition->Weather.PrecipitationRange.GetRandomValue(WeatherRandomStream);
	CurrentWeather.WindPercent = PlanetDefinition->Weather.WindRange.GetRandomValue(WeatherRandomStream);
	CurrentWeather.SunPercent = PlanetDefinition->Weather.SunRange.GetRandomValue(WeatherRandomStream);
	CurrentWeather.Clamp();
	ApplyWeatherCoherence(CurrentWeather);
	TargetWeather = CurrentWeather;
	TransitionStartWeather = CurrentWeather;
	PhaseElapsed = 0.0f;
	bTransitioning = false;
	BroadcastWeather();

	// Start changing immediately. Waiting happens after a transition, not before
	// the player has seen the system move for the first time.
	if (bSimulateWeather)
	{
		GenerateNextWeather();
	}
}

void APlanetWeatherManager::BroadcastWeather()
{
	OnWeatherChanged.Broadcast(CurrentWeather);
}

void APlanetWeatherManager::ApplyWeatherCoherence(FPlanetWeatherState& Weather) const
{
	if (!PlanetDefinition)
	{
		return;
	}

	const float MaximumSunForRain = FMath::Clamp(
		100.0f - Weather.PrecipitationPercent * PlanetDefinition->Weather.SunPenaltyPerMmOfRain,
		0.0f,
		100.0f);
	Weather.SunPercent = FMath::Min(Weather.SunPercent, MaximumSunForRain);
}
