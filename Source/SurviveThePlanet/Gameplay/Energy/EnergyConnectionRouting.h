#pragma once
#include "CoreMinimal.h"

// A bounded visibility graph produces straight cable spans around reserved building footprints.
struct FEnergyConnectionRouting
{
 static bool IsSegmentClear(const FVector2D& Start, const FVector2D& End, const TArray<FBox2D>& Obstacles);
 static bool FindPath(const FVector2D& Start, const FVector2D& End, const TArray<FBox2D>& Obstacles, TArray<FVector2D>& OutPath);
};
