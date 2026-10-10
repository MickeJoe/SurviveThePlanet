#include "PlanetDefinition.h"

float FWeatherPercentageRange::GetRandomValue(FRandomStream& RandomStream) const
{
	const float ClampedMinimum = FMath::Max(FMath::Min(Minimum, Maximum), 0.0f);
	const float ClampedMaximum = FMath::Max(FMath::Max(Minimum, Maximum), 0.0f);
	return RandomStream.FRandRange(ClampedMinimum, ClampedMaximum);
}

double UPlanetDefinition::GetDayLengthMinutes() const
{
	return FMath::Max(60.0, FMath::RoundToDouble(FMath::Max(DayLengthHours, 1.0f) * 60.0));
}

float UPlanetDefinition::GetSolarElevation(double TotalGameMinutes) const
{
	const double HourAngle = (FMath::Fmod(TotalGameMinutes, GetDayLengthMinutes()) / GetDayLengthMinutes() - 0.5) * 2.0 * PI;
	const double Latitude = FMath::DegreesToRadians(FMath::Clamp(LandingLatitudeDegrees, -89.0f, 89.0f));
	const double Declination = FMath::DegreesToRadians(FMath::Clamp(SolarDeclinationDegrees, -45.0f, 45.0f));
	return FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FMath::Sin(Latitude) * FMath::Sin(Declination)
		+ FMath::Cos(Latitude) * FMath::Cos(Declination) * FMath::Cos(HourAngle), -1.0, 1.0)));
}

float UPlanetDefinition::GetLightHours(float MinimumSolarElevation) const
{
	const double Latitude = FMath::DegreesToRadians(FMath::Clamp(LandingLatitudeDegrees, -89.0f, 89.0f));
	const double Declination = FMath::DegreesToRadians(FMath::Clamp(SolarDeclinationDegrees, -45.0f, 45.0f));
	const double Threshold = FMath::Sin(FMath::DegreesToRadians(FMath::Clamp(MinimumSolarElevation, -90.0f, 90.0f)));
	const double CosHourAngle = (Threshold - FMath::Sin(Latitude) * FMath::Sin(Declination))
		/ (FMath::Cos(Latitude) * FMath::Cos(Declination));
	const double LightFraction = FMath::Acos(FMath::Clamp(CosHourAngle, -1.0, 1.0)) / PI;
	return GetDayLengthMinutes() / 60.0 * LightFraction;
}
