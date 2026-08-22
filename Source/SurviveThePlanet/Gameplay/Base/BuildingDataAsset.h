#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/Drones/DroneDataAsset.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "BuildingDataAsset.generated.h"

class UStaticMesh;
class UTexture2D;
class ABaseBuilding;

UENUM(BlueprintType)
enum class ESTPBuildingType : uint8
{
	BaseModule UMETA(DisplayName = "Base Module"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	EnergyStorage UMETA(DisplayName = "Energy Storage"),
	MiningMachine UMETA(DisplayName = "Mining Machine"),
	Other UMETA(DisplayName = "Other"),
	WaterCollector UMETA(DisplayName = "Water Collector"),
	ConcretePlant UMETA(DisplayName = "Concrete Plant"),
	CommunicationModule UMETA(DisplayName = "Communication Module"),
	CargoBay UMETA(DisplayName = "Cargo Bay")
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
	/** Stable toolbar/placement identifier used by the building catalog. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
	ESTPBuildTool BuildTool = ESTPBuildTool::None;

	/** Blueprint or native actor spawned for previews and completed buildings. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
	TSoftClassPtr<ABaseBuilding> BuildingClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
	bool bShowInBuildToolbar = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
	int32 ToolbarSortOrder = 0;

	/** Optional compact toolbar art; Thumbnail is used when unset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Catalog")
	TObjectPtr<UTexture2D> ToolbarIcon;

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

/** Configuration for a building whose water output scales with current rainfall. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UWaterCollectorBuildingDataAsset : public UBuildingDataAsset
{
	GENERATED_BODY()

public:
	UWaterCollectorBuildingDataAsset();

	/** Water produced each minute for every millimetre of hourly precipitation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Water Collection", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WaterPerMinutePerMmOfRain = 1.0f;
};

/** Recipe and throughput configuration for a concrete plant. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UConcretePlantBuildingDataAsset : public UBuildingDataAsset
{
	GENERATED_BODY()

public:
	UConcretePlantBuildingDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Concrete Production", meta = (ClampMin = "0.0"))
	float WaterPerCycle = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Concrete Production", meta = (ClampMin = "0.0"))
	float StonePerCycle = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Concrete Production", meta = (ClampMin = "0.0"))
	float ConcretePerCycle = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Concrete Production", meta = (ClampMin = "0.01"))
	float CycleSeconds = 60.0f;
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
