#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetSurfaceManager.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FSTPGridCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Surface|Grid")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Surface|Grid")
	int32 Y = 0;

	FSTPGridCell() = default;

	FSTPGridCell(int32 InX, int32 InY)
		: X(InX)
		, Y(InY)
	{
	}

	explicit FSTPGridCell(FIntPoint Cell)
		: X(Cell.X)
		, Y(Cell.Y)
	{
	}

	FIntPoint ToIntPoint() const
	{
		return FIntPoint(X, Y);
	}
};

USTRUCT(BlueprintType)
struct FSTPGridPlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Planet Surface|Grid")
	bool bValid = false;

	/** Lowest X/Y cell occupied by this object's footprint. */
	UPROPERTY(BlueprintReadOnly, Category = "Planet Surface|Grid")
	FSTPGridCell OriginCell;

	UPROPERTY(BlueprintReadOnly, Category = "Planet Surface|Grid")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Planet Surface|Grid")
	FRotator WorldRotation = FRotator::ZeroRotator;
};

UCLASS()
class SURVIVETHEPLANET_API APlanetSurfaceManager : public AActor
{
	GENERATED_BODY()

public:
	APlanetSurfaceManager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

	UFUNCTION(CallInEditor, Category = "Planet Surface")
	void RebuildSurface();

	UFUNCTION(CallInEditor, Category = "Planet Surface")
	void ClearSurface();

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	FSTPGridPlacement GetPlacementForWorldLocation(const FVector& WorldLocation, FIntPoint Footprint) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	bool CanOccupyCells(FSTPGridCell OriginCell, FIntPoint Footprint) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	bool ReserveCells(AActor* Occupier, FSTPGridCell OriginCell, FIntPoint Footprint);

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	void ReleaseCells(AActor* Occupier);

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	bool FindNearestFreeCellAdjacentToActor(AActor* Actor, FIntPoint Footprint, FSTPGridCell& OutCell, FVector& OutWorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	bool TryGetActorOriginCell(AActor* Actor, FSTPGridCell& OutOriginCell) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Surface|Grid")
	bool FindNearestFreeCellAdjacentToFootprint(FSTPGridCell OriginCell, FIntPoint OccupiedFootprint, FIntPoint SearchFootprint, FSTPGridCell& OutCell, FVector& OutWorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Planet Surface|Grid")
	float GetTileSpacing() const { return TileSpacing; }

	UFUNCTION(BlueprintPure, Category = "Planet Surface|Grid")
	bool GetCellForWorldLocation(const FVector& WorldLocation, FSTPGridCell& OutCell) const;

	UFUNCTION(BlueprintPure, Category = "Planet Surface|Grid")
	FVector GetWorldLocationForCell(FSTPGridCell Cell) const;

	UFUNCTION(BlueprintPure, Category = "Planet Surface|Grid")
	bool IsCellInBounds(FSTPGridCell Cell) const;

protected:
	/** Legacy per-cell tile class. Kept temporarily so existing Blueprint defaults remain loadable. */
	UPROPERTY(EditAnywhere, Category = "Planet Surface|Legacy")
	TSubclassOf<AActor> TileClass;

	/** Number of logical 100 cm cells represented by one visual chunk edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks", meta = (ClampMin = "1", UIMin = "1"))
	int32 CellsPerChunk = 20;

	/** Diameter of the approximately circular world, measured in whole chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks", meta = (ClampMin = "1", UIMin = "1"))
	int32 ChunkDiameter = 10;

	/** Candidate meshes. A deterministic random choice is made for every spawned chunk. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks")
	TArray<TObjectPtr<UStaticMesh>> ChunkMeshes;

	/** XY size of an imported chunk mesh before actor/component scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float ChunkMeshNativeSize = 2000.0f;

	/** Deterministic seed; identical settings and seed generate identical mesh choices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks")
	int32 ChunkRandomSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Chunks", meta = (Units = "cm"))
	float ChunkHeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet Surface|Grid", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float TileSpacing = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Planet Surface|Grid")
	bool bCenterGridOnActor = true;

	/** Rebuild visual chunks whenever an instance is edited in the level. */
	UPROPERTY(EditAnywhere, Category = "Planet Surface|Chunks")
	bool bBuildInConstructionScript = true;

	/** Derived from CellsPerChunk * ChunkDiameter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface|Grid")
	int32 GridWidth = 200;

	/** Derived from CellsPerChunk * ChunkDiameter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet Surface|Grid")
	int32 GridHeight = 200;

	UPROPERTY(VisibleInstanceOnly, Category = "Planet Surface|Legacy")
	TArray<TObjectPtr<AActor>> SpawnedTiles;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Planet Surface|Chunks")
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedChunkComponents;

	UPROPERTY(VisibleInstanceOnly, Category = "Planet Surface|Grid")
	TMap<int32, TObjectPtr<AActor>> OccupiedCells;

private:
	void SpawnSurface();
	void SpawnChunks();
	void ClearChunkComponents();
	void UpdateDerivedGridSize();
	bool IsChunkInWorld(int32 ChunkX, int32 ChunkY) const;
	bool IsCellPlayable(FSTPGridCell Cell) const;
	FVector2D GetGridOffset() const;
	FVector GetWorldLocationForOriginCell(FSTPGridCell OriginCell, FIntPoint Footprint) const;
	bool FindOccupiedCellsForActor(AActor* Actor, TArray<FSTPGridCell>& OutCells) const;
	int32 MakeCellKey(FSTPGridCell Cell) const;
	FIntPoint SanitizeFootprint(FIntPoint Footprint) const;
};
