#pragma once

#include "CoreMinimal.h"
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

UENUM(BlueprintType)
enum class ESTPBuildingType : uint8
{
	BaseModule UMETA(DisplayName = "Base Module"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	MiningMachine UMETA(DisplayName = "Mining Machine"),
	Other UMETA(DisplayName = "Other")
};

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
	FIntPoint GetGridFootprint() const { return GridFootprint; }

	UFUNCTION(BlueprintPure, Category = "Base Building")
	ESTPBuildingType GetBuildingType() const { return BuildingType; }

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	FText GetBuildingDisplayName() const { return BuildingDisplayName; }

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	FText GetBuildingDescription() const { return BuildingDescription; }

	UFUNCTION(BlueprintPure, Category = "Base Building|UI")
	UTexture2D* GetBuildingThumbnail() const { return BuildingThumbnail; }

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	int32 GetMaxDroneSlots() const { return MaxDroneSlots; }

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

	UFUNCTION(BlueprintPure, Category = "Base Building|Drone Slots")
	ESTPDroneWorkType GetDroneWorkType() const { return DroneWorkType; }

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
	const TArray<FResourceCost>& GetConstructionCosts() const { return ConstructionCosts; }

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyCapacity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ConstructionProgress = 1.0f;

	/** Resources consumed when this building is constructed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Construction", meta = (TitleProperty = "Resource"))
	TArray<FResourceCost> ConstructionCosts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridFootprint = FIntPoint(2, 2);

private:
	void ConfigureMesh();
	void RefreshConstructionProgressBar();
};
