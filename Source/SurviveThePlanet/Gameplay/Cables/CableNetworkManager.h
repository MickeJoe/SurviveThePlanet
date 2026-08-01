#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "CableNetworkManager.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class ABaseBuilding;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSTPCableNetworkChangedSignature);

USTRUCT()
struct FSTPCableCell
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(SaveGame)
	uint8 Connections = 0;
};

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ACableNetworkManager : public AActor
{
	GENERATED_BODY()

public:
	ACableNetworkManager();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Cable")
	bool BeginCableDrag(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Cable")
	bool UpdateCableDrag(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Cable")
	void EndCableDrag();

	UFUNCTION(BlueprintPure, Category = "Cable")
	bool IsDraggingCable() const { return bIsDragging; }

	/** True when the building's connector component reaches a completed Headquarters. */
	UFUNCTION(BlueprintPure, Category = "Cable|Power")
	bool IsBuildingConnectedToPowerGrid(const ABaseBuilding* Building) const;

	UFUNCTION(BlueprintPure, Category = "Cable|Power")
	bool IsBuildingOperational(const ABaseBuilding* Building) const;

	UFUNCTION(BlueprintPure, Category = "Cable|Power")
	float GetGridProductionPerMinute() const { return GridProductionPerMinute; }

	UFUNCTION(BlueprintPure, Category = "Cable|Power")
	float GetGridConsumptionPerMinute() const { return GridConsumptionPerMinute; }

	UFUNCTION(BlueprintPure, Category = "Cable|Power")
	float GetGridStorageCapacity() const { return GridStorageCapacity; }

	UFUNCTION(BlueprintCallable, Category = "Cable|Power")
	void RefreshEnergyGrid();

	UFUNCTION(BlueprintPure, Category = "Cable|Grid")
	bool HasConnectorAtCell(FSTPGridCell Cell) const;

	UFUNCTION(BlueprintCallable, Category = "Cable|Grid")
	void GetConnectorCells(TArray<FSTPGridCell>& OutCells) const;

	UPROPERTY(BlueprintAssignable, Category = "Cable")
	FSTPCableNetworkChangedSignature OnCableNetworkChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cable")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	TObjectPtr<UStaticMesh> CableStraightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	TObjectPtr<UStaticMesh> CableTurnMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	TObjectPtr<UStaticMesh> CableTMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	TObjectPtr<UStaticMesh> Cable4Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float StraightBaseYaw = 90.0f;

	/** Optional pitch correction for imported straight connector meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float StraightBasePitch = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float StraightBaseRoll = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	FVector StraightMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float TurnBaseYaw = 270.0f;

	/** Per-mesh scale correction; standardized connector assets use unit scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	FVector TurnMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float TBaseYaw = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Meshes")
	float FourWayBaseYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cable|Placement")
	float CableHeightOffset = 5.0f;

private:
	enum : uint8
	{
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3
	};

	/** Canonical connector occupancy and topology. SaveGame keeps grid cells for future saves. */
	UPROPERTY(SaveGame)
	TMap<FIntPoint, FSTPCableCell> CableCells;

	UPROPERTY(Transient)
	TObjectPtr<APlanetSurfaceManager> SurfaceManager;

	bool bIsDragging = false;
	FIntPoint DragStartCell = FIntPoint::ZeroValue;
	FIntPoint LastDragCell = FIntPoint::ZeroValue;
	TMap<FIntPoint, uint8> ConnectionsBeforeDrag;
	float GridRefreshAccumulator = 0.0f;
	float PendingEnergyDelta = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Cable|Power")
	float GridProductionPerMinute = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Cable|Power")
	float GridConsumptionPerMinute = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Cable|Power")
	float GridStorageCapacity = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Cable|Power")
	bool bCanSupplyAllConsumers = true;

	APlanetSurfaceManager* ResolveSurfaceManager();
	bool TryGetCell(const FVector& WorldLocation, FIntPoint& OutCell);
	void RestoreNetworkBeforeDrag();
	void AddPathBetweenCells(const FIntPoint& From, const FIntPoint& To);
	void ConnectAdjacentCells(const FIntPoint& From, const FIntPoint& To);
	void AddConnection(const FIntPoint& Cell, uint8 Direction);
	void RefreshCableCell(const FIntPoint& Cell);
	FSTPCableCell& FindOrAddCableCell(const FIntPoint& Cell);
	void GetTouchingCableCells(const ABaseBuilding* Building, TArray<FIntPoint>& OutCells) const;
	void GetHeadquartersNetworkCells(TSet<FIntPoint>& OutCells) const;
};
