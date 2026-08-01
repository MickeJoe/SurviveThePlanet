#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/Drones/DroneDataAsset.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "BuildingDataAsset.generated.h"

class UStaticMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class ESTPBuildingType : uint8
{
	BaseModule UMETA(DisplayName = "Base Module"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	EnergyStorage UMETA(DisplayName = "Energy Storage"),
	MiningMachine UMETA(DisplayName = "Mining Machine"),
	Other UMETA(DisplayName = "Other")
};

/** Resource produced by one 100%-efficient drone during one minute. */
USTRUCT(BlueprintType)
struct FSTPResourceOutputRate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	EResourceType Resource = EResourceType::Iron;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AmountPerMinutePerDroneAt100Percent = 0.0f;
};

/** Shared identity, visuals and balance data for one building type. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UBuildingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultBuildingDataName", "Building");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName BuildingTag = TEXT("Building");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	ESTPBuildingType BuildingType = ESTPBuildingType::Other;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> BuildingMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyConsumptionPerMinute = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyProductionPerMinute = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyStorageCapacity = 0.0f;

	/** When disabled, the native building class defaults are used for all three energy values. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Energy")
	bool bOverrideEnergySettings = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Construction", meta = (TitleProperty = "Resource"))
	TArray<FResourceCost> ConstructionCosts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone Slots", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxDroneSlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone Slots", meta = (ClampMin = "0", UIMin = "0"))
	int32 InitiallyUnlockedDroneSlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone Slots")
	ESTPDroneWorkType DroneWorkType = ESTPDroneWorkType::Construction;
};

/** Configuration asset for a building that expands the connected HQ grid's storage. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UEnergyStorageBuildingDataAsset : public UBuildingDataAsset
{
	GENERATED_BODY()

public:
	UEnergyStorageBuildingDataAsset();
};

/** Mining-only configuration kept out of unrelated building data assets. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UMiningBuildingDataAsset : public UBuildingDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining")
	TArray<EResourceType> SupportedResourceTypes;

	/** Output scales linearly with each assigned drone's Mining efficiency. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining|Output", meta = (TitleProperty = "Resource"))
	TArray<FSTPResourceOutputRate> OutputPerDroneAt100Percent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining|Placement")
	FTransform SourceTransformOffset = FTransform::Identity;
};
