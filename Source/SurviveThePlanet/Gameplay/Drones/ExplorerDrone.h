#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "ExplorerDrone.generated.h"

class AHexSectorGrid;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FExplorerDroneProgressSignature, int32, ScannedSectors, int32, TotalSectors);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FExplorerDroneCompletedSignature);

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AExplorerDrone : public ABaseDrone
{
	GENERATED_BODY()

public:
	AExplorerDrone();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Explorer Drone") bool ActivateExploration();
	UFUNCTION(BlueprintPure, Category="Explorer Drone") bool CanActivateExploration() const;
	UFUNCTION(BlueprintPure, Category="Explorer Drone") bool IsExploring() const { return bIsExploring; }
	UFUNCTION(BlueprintPure, Category="Explorer Drone") int32 GetScannedSectorCount() const { return ScannedSectorCount; }
	UFUNCTION(BlueprintPure, Category="Explorer Drone") int32 GetTotalScanCount() const { return TargetSectorIds.Num(); }
	UFUNCTION(BlueprintPure, Category="Explorer Drone") TArray<int32> GetPreviewSectorIds() const;
	UFUNCTION(BlueprintCallable, Category="Explorer Drone|UI") void SetActivationPreviewVisible(bool bVisible);

	UPROPERTY(BlueprintAssignable, Category="Explorer Drone") FExplorerDroneProgressSignature OnExplorationProgress;
	UPROPERTY(BlueprintAssignable, Category="Explorer Drone") FExplorerDroneCompletedSignature OnExplorationCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explorer Drone", meta=(ClampMin="1", UIMin="1")) int32 SectorsPerMission = 3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explorer Drone", meta=(ClampMin="1.0", Units="cm/s")) float ExplorationFlightSpeed = 900.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explorer Drone", meta=(ClampMin="1.0", Units="cm")) float ArrivalRadius = 100.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explorer Drone", meta=(Units="cm")) float FlightHeight = 240.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explorer Drone|UI") FLinearColor PreviewColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);

private:
	AHexSectorGrid* FindSectorGrid() const;
	void RefreshTargets();
	void DrawActivationPreview() const;
	void FinishExploration();

	UPROPERTY(Transient) TObjectPtr<AHexSectorGrid> SectorGrid;
	UPROPERTY(Transient) TArray<int32> TargetSectorIds;
	int32 CurrentTargetIndex = 0;
	int32 ScannedSectorCount = 0;
	bool bIsExploring = false;
	bool bShowActivationPreview = false;
};
