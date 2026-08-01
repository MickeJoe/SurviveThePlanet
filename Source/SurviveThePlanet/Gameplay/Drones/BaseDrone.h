#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Drones/DroneDataAsset.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Work/ConstructionJobQueueSubsystem.h"
#include "Gameplay/SelectableWorldActor.h"
#include "BaseDrone.generated.h"

class ABaseBuilding;
class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESTPConstructionJobPhase : uint8
{
	None,
	TravelToBuilding,
	ConstructBuilding
};

USTRUCT(BlueprintType)
struct FSTPOngoingConstructionJob
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Job")
	FSTPConstructionJob Job;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Job")
	ESTPConstructionJobPhase Phase = ESTPConstructionJobPhase::None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroneBuildingAssignmentChangedSignature, ABaseBuilding*, AssignedBuilding);

/** Shared editor data and capabilities for every drone type. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ABaseDrone : public ASelectableWorldActor
{
	GENERATED_BODY()

public:
	ABaseDrone();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetGridFootprint() const;

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

	UFUNCTION(BlueprintPure, Category = "Drone|Construction")
	bool HasOngoingConstructionJob() const { return bHasOngoingConstructionJob; }

	UFUNCTION(BlueprintPure, Category = "Drone|Construction")
	ABaseBuilding* GetConstructionTarget() const { return OngoingConstructionJob.Job.TargetBuilding; }

	UFUNCTION(BlueprintCallable, Category = "Drone|Construction")
	bool AssignConstructionJob(const FSTPConstructionJob& Job);

	UFUNCTION(BlueprintCallable, Category = "Drone|Construction")
	void TickOngoingConstructionJob(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Drone|Construction")
	void ClearOngoingConstructionJob();

	UFUNCTION(BlueprintPure, Category = "Drone|Idle")
	bool HasIdleDestination() const { return bHasIdleDestination; }

	UFUNCTION(BlueprintCallable, Category = "Drone|Idle")
	void SetIdleDestination(FSTPGridCell Cell, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Drone|Idle")
	void ClearIdleDestination();

	UFUNCTION(BlueprintCallable, Category = "Drone|Idle")
	void TickIdleMovement(float DeltaSeconds);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DroneMesh;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float WorkRange = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drone|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DroneVisualScale = 0.75f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Construction")
	bool bHasOngoingConstructionJob = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Construction")
	FSTPOngoingConstructionJob OngoingConstructionJob;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Idle")
	bool bHasIdleDestination = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Idle")
	FSTPGridCell IdleCell;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Idle")
	FVector IdleDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	TArray<FVector> IdlePath;

	int32 IdlePathIndex = 0;

private:
	friend class ABaseBuilding;
	void SetBuildingAssignmentInternal(ABaseBuilding* Building, int32 SlotIndex);
	void ParkAtAssignedBuilding();
	void ConfigureMesh();
	void ApplyVisualScale();
	bool IsOngoingConstructionJobValid() const;
	void TickTravelToBuilding(float DeltaSeconds);
	void TickBuildTarget(float DeltaSeconds);
};
