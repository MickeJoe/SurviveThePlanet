#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Energy UMETA(DisplayName = "Energy"),
	Iron UMETA(DisplayName = "Iron Ore"),
	ControlChip UMETA(DisplayName = "Circuit Boards"),
	Copper UMETA(DisplayName = "Copper Ore"),
	Stone UMETA(DisplayName = "Stone"),
	Water UMETA(DisplayName = "Water"),
	Concrete UMETA(DisplayName = "Concrete"),
	Steel UMETA(DisplayName = "Steel"),
	Coal UMETA(DisplayName = "Carbon"),
	Polymer UMETA(DisplayName = "Polymer"),
	Connector UMETA(DisplayName = "Connectors"),
	// Append new values: existing Blueprint and inventory IDs must remain stable.
	Silica UMETA(DisplayName = "Silica"),
	RareMinerals UMETA(DisplayName = "Rare Minerals"),
	UraniumOre UMETA(DisplayName = "Uranium Ore"),
	AlienMaterial UMETA(DisplayName = "Alien Material"),
	CopperMetal UMETA(DisplayName = "Copper"),
	Glass UMETA(DisplayName = "Glass"),
	PurifiedWater UMETA(DisplayName = "Purified Water"),
	Ceramics UMETA(DisplayName = "Ceramics"),
	RareMetals UMETA(DisplayName = "Rare Metals"),
	SyntheticFuel UMETA(DisplayName = "Synthetic Fuel"),
	FuelRods UMETA(DisplayName = "Fuel Rods"),
	XenoCompound UMETA(DisplayName = "Xeno Compound"),
	MachineParts UMETA(DisplayName = "Machine Parts"),
	StructuralComponents UMETA(DisplayName = "Structural Components"),
	CompositePanels UMETA(DisplayName = "Composite Panels"),
	BatteryCells UMETA(DisplayName = "Battery Cells"),
	OpticalUnits UMETA(DisplayName = "Optical Units"),
	CoolingUnits UMETA(DisplayName = "Cooling Units"),
	PressureUnits UMETA(DisplayName = "Pressure Units"),
	SensorUnits UMETA(DisplayName = "Sensor Units"),
	PowerRegulators UMETA(DisplayName = "Power Regulators"),
	FuelCells UMETA(DisplayName = "Fuel Cells"),
	IndustrialTools UMETA(DisplayName = "Industrial Tools"),
	DroneSpareParts UMETA(DisplayName = "Drone Spare Parts"),
	AdvancedAlloy UMETA(DisplayName = "Advanced Alloy"),
	ControlUnits UMETA(DisplayName = "Control Units"),
	NavigationCores UMETA(DisplayName = "Navigation Cores"),
	LifeSupportModules UMETA(DisplayName = "Life Support Modules"),
	ReactorAssemblies UMETA(DisplayName = "Reactor Assemblies"),
	PowerPacks UMETA(DisplayName = "Power Packs"),
	SurveyPackages UMETA(DisplayName = "Survey Packages"),
	CommunicationArrays UMETA(DisplayName = "Communication Arrays"),
	PrecisionComponents UMETA(DisplayName = "Precision Components"),
	HabitatModules UMETA(DisplayName = "Habitat Modules"),
	XenotechModules UMETA(DisplayName = "Xenotech Modules")
};

/** A resource and the amount required for a purchase or construction. */
USTRUCT(BlueprintType)
struct FResourceCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	EResourceType Resource = EResourceType::Iron;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources", meta = (ClampMin = "0", UIMin = "0"))
	int32 Cost = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FResourceAmountChangedSignature,
	EResourceType, ResourceType,
	int32, NewAmount);

UCLASS()
class SURVIVETHEPLANET_API AResourceManager : public AActor
{
	GENERATED_BODY()

public:
	AResourceManager();

	/** Returns the current amount of a resource. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetResourceAmount(EResourceType ResourceType) const;

	/** Replaces the current amount. Negative values are clamped to zero. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetResourceAmount(EResourceType ResourceType, int32 NewAmount);

	/** Adds to the current amount. The result is never allowed below zero. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddResource(EResourceType ResourceType, int32 Amount);

	/** Returns true when at least Amount units are available. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool HasResource(EResourceType ResourceType, int32 Amount) const;

	/** Spends Amount units if available and reports whether it succeeded. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool TrySpendResource(EResourceType ResourceType, int32 Amount);

	/** Returns true when all costs can be paid. Duplicate resource rows are combined. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanAffordCosts(const TArray<FResourceCost>& Costs) const;

	/** Atomically pays all costs. Nothing is spent unless every cost can be paid. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool TrySpendCosts(const TArray<FResourceCost>& Costs);

	UFUNCTION(BlueprintPure, Category = "Resources|Energy")
	int32 GetEnergyStorageCapacity() const { return EnergyStorageCapacity; }

	UFUNCTION(BlueprintCallable, Category = "Resources|Energy")
	void SetEnergyStorageCapacity(int32 NewCapacity);

	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FResourceAmountChangedSignature OnResourceAmountChanged;

protected:
	virtual void BeginPlay() override;

	/** Resource amounts copied into the runtime inventory when play starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources", meta = (ClampMin = "0", UIMin = "0"))
	TMap<EResourceType, int32> InitialResourceAmounts;

	/** Current resource amounts during play. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resources")
	TMap<EResourceType, int32> ResourceAmounts;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resources|Energy")
	int32 EnergyStorageCapacity = 1000;
};
