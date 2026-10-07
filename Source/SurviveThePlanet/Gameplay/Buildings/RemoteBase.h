#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "RemoteBase.generated.h"

class UEnergyConnectionComponent;
class UEnergyCoverageComponent;
class AHexSectorGrid;

/** Sector outpost using the shared construction, placement and power-relay systems. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ARemoteBase : public ABaseBuilding
{
	GENERATED_BODY()
public:
	ARemoteBase();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetConstructionProgress(float NewProgress) override;
	virtual void SetPlacementPreview(bool bPreview) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEnergyConnectionComponent> PowerConnection;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEnergyCoverageComponent> EnergyCoverage;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Antenna;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Perimeter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Scan;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	TWeakObjectPtr<AHexSectorGrid> SectorGrid;
	int32 EstablishedSectorId = INDEX_NONE;
	void ReleaseSector();
	void RefreshSector();
	void RefreshVisuals();
};
