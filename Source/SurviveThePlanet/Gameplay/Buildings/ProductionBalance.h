#pragma once

#include "CoreMinimal.h"

// Material production balance, independent of the player's simulation speed.
namespace STPProductionBalance
{
    constexpr float SpeedMultiplier = 3.0f;

    inline float CycleDuration(float BaseSeconds)
    {
        return FMath::Max(0.1f, BaseSeconds / SpeedMultiplier);
    }

    inline float OutputRate(float BasePerMinute)
    {
        return FMath::Max(0.0f, BasePerMinute) * SpeedMultiplier;
    }
}
