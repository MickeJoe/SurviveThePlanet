#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlanetSectorTemplate.generated.h"

class UPlanetTerrainClusterShape;

USTRUCT(BlueprintType)
struct SURVIVETHEPLANET_API FPlanetSectorClusterSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	TObjectPtr<UPlanetTerrainClusterShape> Shape = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	FName SlotId = NAME_None;
};

/** Baked, runtime-independent description of an authored sector layout. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetSectorTemplate : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	FName TemplateId = NAME_None;

	/** Explicit opt-in. Ordinary sectors may use any template, including these. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template|Generation")
	bool bCanBeStartingSector = false;

	/** Stable selection independent of catalog order; never substitutes an ineligible start. */
	static UPlanetSectorTemplate* SelectForSector(const TArray<UPlanetSectorTemplate*>& Templates,
		int32 WorldSeed, int32 SectorId, bool bStartingSector);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template", meta = (ClampMin = "1.0"))
	FVector2D SectorSize = FVector2D(6928.203, 8000.0);

	/** Center-to-corner radius matching AHexSectorGrid::ExplorationSectorRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template", meta = (ClampMin = "100.0", Units = "cm"))
	float SectorRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector Template")
	TArray<FPlanetSectorClusterSlot> ClusterSlots;
};
