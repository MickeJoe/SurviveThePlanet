#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexSectorGrid.generated.h"

UENUM(BlueprintType)
enum class ESectorState : uint8
{
	Undiscovered,
	Discovered,
	Established
};

USTRUCT(BlueprintType)
struct FHexSector
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") int32 Id = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") int32 Q = 0;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") int32 R = 0;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") int32 Ring = 0;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") FVector WorldCenter = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") TArray<int32> NeighborIds;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector") ESectorState State = ESectorState::Undiscovered;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSectorStateChangedSignature, int32, SectorId, ESectorState, NewState);

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AHexSectorGrid : public AActor
{
	GENERATED_BODY()

public:
	AHexSectorGrid();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Corner-to-center radius of one large exploration sector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector", meta=(ClampMin="100.0", Units="cm")) float ExplorationSectorRadius = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector", meta=(ClampMin="0", ClampMax="20")) int32 GridRadius = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector") bool bEstablishStartingSector = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector", meta=(ClampMin="0")) int32 StartingSectorId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector") float LineHeightOffset = 35.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector") float LineThickness = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector") bool bShowWorldGrid = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Debug") FLinearColor UndiscoveredColor = FLinearColor(0.0f, 0.45f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Debug") FLinearColor DiscoveredColor = FLinearColor(0.0f, 0.85f, 1.0f, 0.45f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Debug") FLinearColor EstablishedColor = FLinearColor(0.15f, 1.0f, 0.25f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Fog", meta=(ClampMin="0.0", ClampMax="1.0")) float UndiscoveredFogOpacity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Fog", meta=(ClampMin="0.0", ClampMax="1.0")) float DiscoveredFogOpacity = 0.55f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hex Sector") TArray<FHexSector> Sectors;

	UFUNCTION(BlueprintCallable, Category="Hex Sector") void RebuildGrid();
	UFUNCTION(BlueprintPure, Category="Hex Sector") int32 GetSectorAtWorldLocation(FVector WorldLocation) const;
	UFUNCTION(BlueprintPure, Category="Hex Sector") bool GetSectorById(int32 SectorId, FHexSector& OutSector) const;
	UFUNCTION(BlueprintPure, Category="Hex Sector") TArray<int32> GetNeighborIds(int32 SectorId) const;
	UFUNCTION(BlueprintPure, Category="Hex Sector|Exploration") TArray<int32> GetNearestUndiscoveredSectorIds(FVector Origin, int32 MaxCount = 3) const;
	/** Manual/debug entry point only. Exploration gameplay will call this later. */
	UFUNCTION(BlueprintCallable, Category="Hex Sector") bool SetSectorState(int32 SectorId, ESectorState NewState);
	UFUNCTION(BlueprintPure, Category="Hex Sector|Fog") float GetFogOpacityForSector(int32 SectorId) const;

	UPROPERTY(BlueprintAssignable, Category="Hex Sector|Exploration")
	FSectorStateChangedSignature OnSectorStateChanged;

private:
	FIntPoint RoundAxial(const FVector2D& LocalPosition) const;
	FVector AxialToWorld(int32 Q, int32 R) const;
	FColor GetDebugColor(ESectorState State) const;
	void DrawGrid() const;
};
