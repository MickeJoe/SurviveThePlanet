#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Work/ConstructionJobQueueSubsystem.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "ConstructionDrone.generated.h"

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

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AConstructionDrone : public ABaseDrone
{
	GENERATED_BODY()

public:
	AConstructionDrone();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetGridFootprint() const { return GridFootprint; }

	UFUNCTION(BlueprintPure, Category = "Construction Drone|Jobs")
	bool HasOngoingConstructionJob() const { return bHasOngoingConstructionJob; }

	UFUNCTION(BlueprintPure, Category = "Construction Drone|Jobs")
	ABaseBuilding* GetConstructionTarget() const { return OngoingConstructionJob.Job.TargetBuilding; }

	UFUNCTION(BlueprintPure, Category = "Construction Drone|Idle")
	bool HasIdleDestination() const { return bHasIdleDestination; }

	virtual bool IsAvailableForAssignment() const override;

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Jobs")
	bool AssignConstructionJob(const FSTPConstructionJob& Job);

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Jobs")
	void TickOngoingConstructionJob(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Jobs")
	void ClearOngoingConstructionJob();

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Idle")
	void SetIdleDestination(FSTPGridCell Cell, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Idle")
	void ClearIdleDestination();

	UFUNCTION(BlueprintCallable, Category = "Construction Drone|Idle")
	void TickIdleMovement(float DeltaSeconds);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleBuildingAssignmentChanged(bool bIsAssigned) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DroneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone")
	FName DroneTypeTag = TEXT("Construction");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction Drone|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction Drone|Movement", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float WorkRange = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DroneVisualScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridFootprint = FIntPoint(1, 1);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Drone|Jobs")
	bool bHasOngoingConstructionJob = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Drone|Jobs")
	FSTPOngoingConstructionJob OngoingConstructionJob;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Drone|Idle")
	bool bHasIdleDestination = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Drone|Idle")
	FSTPGridCell IdleCell;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Construction Drone|Idle")
	FVector IdleDestination = FVector::ZeroVector;

private:
	void ConfigureMesh();
	void ApplyVisualScale();
	bool IsOngoingConstructionJobValid() const;
	void TickTravelToBuilding(float DeltaSeconds);
	void TickBuildTarget(float DeltaSeconds);
};
