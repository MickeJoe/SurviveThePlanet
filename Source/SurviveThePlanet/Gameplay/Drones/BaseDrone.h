#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Drones/DroneDataAsset.h"
#include "Gameplay/SelectableWorldActor.h"
#include "BaseDrone.generated.h"

class ABaseBuilding;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroneBuildingAssignmentChangedSignature, ABaseBuilding*, AssignedBuilding);

/** Shared editor data and capabilities for every drone type. */
UCLASS(Abstract, Blueprintable)
class SURVIVETHEPLANET_API ABaseDrone : public ASelectableWorldActor
{
	GENERATED_BODY()

public:
	ABaseDrone();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Drone|UI")
	FText GetDroneDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Drone|UI")
	UTexture2D* GetDroneThumbnail() const;

	UFUNCTION(BlueprintPure, Category = "Drone|Data")
	UDroneDataAsset* GetDroneData() const { return DroneData; }

	UFUNCTION(BlueprintPure, Category = "Drone|Work")
	float GetWorkingRate(ESTPDroneWorkType WorkType) const;

	UFUNCTION(BlueprintPure, Category = "Drone|Work")
	bool CanPerformWork(ESTPDroneWorkType WorkType) const;

	UFUNCTION(BlueprintPure, Category = "Drone|Assignment")
	ABaseBuilding* GetAssignedBuilding() const { return AssignedBuilding; }

	UFUNCTION(BlueprintPure, Category = "Drone|Assignment")
	int32 GetAssignedBuildingSlot() const { return AssignedBuildingSlot; }

	UFUNCTION(BlueprintPure, Category = "Drone|Assignment")
	bool IsAssignedToBuilding() const { return AssignedBuilding != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Drone|Assignment")
	bool IsParkedAtAssignedBuilding() const { return bParkedAtAssignedBuilding; }

	UFUNCTION(BlueprintPure, Category = "Drone|Assignment")
	virtual bool IsAvailableForAssignment() const;

	UFUNCTION(BlueprintCallable, Category = "Drone|Assignment")
	bool AssignToBuilding(ABaseBuilding* Building, int32 PreferredSlot = -1);

	UFUNCTION(BlueprintCallable, Category = "Drone|Assignment")
	bool UnassignFromBuilding();

	UPROPERTY(BlueprintAssignable, Category = "Drone|Assignment")
	FDroneBuildingAssignmentChangedSignature OnBuildingAssignmentChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleBuildingAssignmentChanged(bool bIsAssigned);

	/** Canonical identity, UI and balance configuration for this drone type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Data")
	TObjectPtr<UDroneDataAsset> DroneData;

	/** Temporary compatibility fallback while existing Blueprints migrate to DroneData. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|UI")
	FText DroneDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultDroneName", "Drone");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|UI")
	TObjectPtr<UTexture2D> DroneThumbnail;

	/** Temporary compatibility fallback. New drone types should configure rates in DroneData. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Work", meta = (ClampMin = "0.0", UIMin = "0.0"))
	TMap<ESTPDroneWorkType, float> WorkingRates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone")
	FName DroneTag = TEXT("Drone");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Assignment")
	TObjectPtr<ABaseBuilding> AssignedBuilding;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Assignment")
	int32 AssignedBuildingSlot = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Assignment")
	bool bParkedAtAssignedBuilding = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Assignment", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
	float AssignmentMoveSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Assignment", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float AssignmentArrivalDistance = 120.0f;

private:
	friend class ABaseBuilding;
	void SetBuildingAssignmentInternal(ABaseBuilding* Building, int32 SlotIndex);
	void ParkAtAssignedBuilding();
};
