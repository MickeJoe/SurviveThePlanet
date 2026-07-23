#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlanetSurfaceManager.generated.h"

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

	UFUNCTION(BlueprintPure, Category = "Planet Surface|Grid")
	float GetTileSpacing() const { return TileSpacing; }

protected:
	UPROPERTY(EditAnywhere, Category = "Planet Surface")
	TSubclassOf<AActor> TileClass;

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridWidth = 25;

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridHeight = 25;

	UPROPERTY(EditAnywhere, Category = "Planet Surface", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float TileSpacing = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Planet Surface")
	bool bCenterGridOnActor = true;

	UPROPERTY(EditAnywhere, Category = "Planet Surface")
	bool bBuildInConstructionScript = true;

	UPROPERTY(VisibleInstanceOnly, Category = "Planet Surface")
	TArray<TObjectPtr<AActor>> SpawnedTiles;

	UPROPERTY(VisibleInstanceOnly, Category = "Planet Surface|Grid")
	TMap<int32, TObjectPtr<AActor>> OccupiedCells;

private:
	void SpawnSurface();
	FVector GetTileLocation(int32 X, int32 Y) const;
	FVector2D GetGridOffset() const;
	FVector GetWorldLocationForOriginCell(FSTPGridCell OriginCell, FIntPoint Footprint) const;
	int32 MakeCellKey(FSTPGridCell Cell) const;
	FIntPoint SanitizeFootprint(FIntPoint Footprint) const;
};