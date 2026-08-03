#include "PlanetDefinition.h"

float FWeatherPercentageRange::GetRandomValue(FRandomStream& RandomStream) const
{
	const float ClampedMinimum = FMath::Max(FMath::Min(Minimum, Maximum), 0.0f);
	const float ClampedMaximum = FMath::Max(FMath::Max(Minimum, Maximum), 0.0f);
	return RandomStream.FRandRange(ClampedMinimum, ClampedMaximum);
}
