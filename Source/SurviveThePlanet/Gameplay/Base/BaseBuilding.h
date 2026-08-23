#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/SelectableWorldActor.h"
#include "BaseBuilding.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UWidgetComponent;
class ABaseDrone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroneSlotsChangedSignature, int32, UnlockedSlots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDroneAssignmentsChangedSignature);

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ABaseBuilding : public ASelectableWorldActor
{
	GENERATED_BODY()

public:
	ABaseBuilding();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void SetConstructionProgress(float NewProgress);

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void ShowConstructionProgress();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void HideConstructionProgress();

	UFUNCTION(BlueprintPure, Category = "Construction")
	float GetConstructionProgress() const { return ConstructionProgress; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetGridFootprint() const;

	/** Computed from connector cells and their route to a completed energy module. */
	UFUNCTION(BlueprintPure, Category = "Base Building|Power")
	bool IsConnectedToPowerGrid() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Power")
	bool IsOperational() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Exploration")
	bool ProvidesVision() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Exploration")
	float GetVisionRadius() const { return FMath::Max(0.0f, VisionRadius); }

	UFUNCTION(BlueprintPure, Category = "Base Building|Power")
	virtual float GetEnergyConsumptionPerMinute() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Power")
	float GetEnergyProductionPerMinute() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Power")
	float GetEnergyStorageCapacity() const;

	UFUNCTION(BlueprintPure, Category = "Base Building")
	ESTPBuildingType GetBuildingType() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Data")
	UBuildingDataAsset* GetBuildingData() const { return BuildingData; }

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	FText GetBuildingDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	FText GetBuildingDescription() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	UTexture2D* GetBuildingThumbnail() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	int32 GetMaxDroneSlots() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	int32 GetUnlockedDroneSlots() const { return UnlockedDroneSlots; }

	UFUNCTION(BlueprintCallable, Category = "Base Building|Drone Slots")
	void SetUnlockedDroneSlots(int32 NewUnlockedSlots);

	UFUNCTION(BlueprintCallable, Category = "Base Building|Drone Slots")
	void UnlockDroneSlots(int32 SlotsToUnlock = 1);

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	ABaseDrone* GetAssignedDroneAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	int32 GetAssignedDroneCount() const;

	/** Sum of the assigned drones' rates for this building's configured work type. */
	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	float GetCombinedDroneEfficiency() const;

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	ESTPDroneWorkType GetDroneWorkType() const;

	UFUNCTION(BlueprintCallable, Category = "Base Building|Drone Slots")
	bool TryAssignDrone(ABaseDrone* Drone, int32 PreferredSlot = -1);

	UFUNCTION(BlueprintCallable, Category = "Base Building|Drone Slots")
	bool UnassignDrone(ABaseDrone* Drone);

	UFUNCTION(BlueprintCallable, Category = "Base Building|Drone Slots")
	bool UnassignDroneAtSlot(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "Base Building|Drone Slots")
	FDroneSlotsChangedSignature OnDroneSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Base Building|Drone Slots")
	FDroneAssignmentsChangedSignature OnDroneAssignmentsChanged;

	UFUNCTION(BlueprintPure, Category = "Construction")
	const TArray<FResourceCost>& GetConstructionCosts() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building|Visuals")
	TObjectPtr<UStaticMesh> BaseModuleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> ConstructionProgressBar;

	/** Canonical identity, visuals and balance configuration for this building type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Data")
	TObjectPtr<UBuildingDataAsset> BuildingData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building")
	FName BuildingTag = TEXT("BaseModule");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building")
	ESTPBuildingType BuildingType = ESTPBuildingType::BaseModule;

	/** Shared content displayed by the building information popup. Override in child Blueprints. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|UI")
	FText BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultBuildingName", "Building");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|UI", meta = (MultiLine = "true"))
	FText BuildingDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|UI")
	TObjectPtr<UTexture2D> BuildingThumbnail;

	/** Total slot capacity at the building's highest upgrade level. Zero hides the drone section. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Drone Slots", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxDroneSlots = 0;

	/** Slots available when a new instance of this building enters play. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Drone Slots", meta = (ClampMin = "0", UIMin = "0"))
	int32 InitiallyUnlockedDroneSlots = 0;

	/** Runtime state changed by building upgrades. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Base Building|Drone Slots", SaveGame)
	int32 UnlockedDroneSlots = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Base Building|Drone Slots")
	TArray<TObjectPtr<ABaseDrone>> AssignedDrones;

	/** Activity used to calculate drone efficiency in this building. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Drone Slots")
	ESTPDroneWorkType DroneWorkType = ESTPDroneWorkType::Construction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 1000.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Base Building")
	float CurrentHealth = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Power", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyConsumptionPerMinute = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Power", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyProductionPerMinute = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base Building|Power", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyStorageCapacity = 1000.0f;

	/** Flat world-space reveal radius. Configure per Blueprint or placed instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building|Exploration", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float VisionRadius = 3333.0f;

	/** Enable on extra reveal buildings; the BaseModule always reveals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building|Exploration")
	bool bProvidesVision = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ConstructionProgress = 1.0f;

	/** Resources consumed when this building is constructed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Construction", meta = (TitleProperty = "Resource"))
	TArray<FResourceCost> ConstructionCosts;

private:
	void ConfigureMesh();
	void RefreshConstructionProgressBar();
	int32 GetInitiallyUnlockedDroneSlots() const;
	float GetMaxHealth() const;
	FName GetBuildingTag() const;
};
