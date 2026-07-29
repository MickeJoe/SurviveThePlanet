#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DroneDataAsset.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ESTPDroneWorkType : uint8
{
	Construction UMETA(DisplayName = "Construction"),
	Mining UMETA(DisplayName = "Mining")
};

/** Shared identity, UI and balance data for one drone type. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UDroneDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UDroneDataAsset();

	UFUNCTION(BlueprintPure, Category = "Drone Data")
	float GetWorkingRate(ESTPDroneWorkType WorkType) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultDroneDataName", "Drone");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> Thumbnail;

	/** Normalized efficiency values: 1.0 = 100%, 0.65 = 65%. Zero disables the work type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Work", meta = (ClampMin = "0.0", UIMin = "0.0"))
	TMap<ESTPDroneWorkType, float> WorkingRates;
};
