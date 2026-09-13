#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexSectorGrid.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class ESectorState : uint8
{
	Undiscovered,
	Discovered,
	Established
};

USTRUCT(BlueprintType)
struct FSectorBuildablePocket
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Extent = FVector2D(800.0f, 800.0f);
};

USTRUCT(BlueprintType)
struct FSectorResourceSlot
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SlotType = TEXT("Any");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Location = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 250.0f;
};

USTRUCT(BlueprintType)
struct FSectorCorridor
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Start = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D End = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Width = 300.0f;
};

USTRUCT(BlueprintType)
struct FSectorTemplateDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TemplateId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UStaticMesh> TerrainMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TSoftObjectPtr<UStaticMesh>> DressingMeshes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> DressingRuleTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSupportsHQ = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowMirroring = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> SafeRotations = {0, 60, 120, 180, 240, 300};
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSectorBuildablePocket> BuildablePockets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> ConnectionSockets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSectorResourceSlot> ResourceSlots;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> LandmarkSlots;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVector2D> SubBaseSlots;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FSectorCorridor> DroneCorridors;
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
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector|Generation") FName TemplateId = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector|Generation") int32 TemplateRotationDegrees = 0;
	UPROPERTY(BlueprintReadOnly, Category="Hex Sector|Generation") bool bTemplateMirrored = false;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Generation") int32 LayoutSeed = 1337;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Generation") TArray<FSectorTemplateDefinition> SectorTemplates;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Sector|Debug") bool bShowTemplateDebug = false;
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
	UFUNCTION(BlueprintPure, Category="Hex Sector|Generation") bool GetTemplateForSector(int32 SectorId, FSectorTemplateDefinition& OutTemplate) const;
	UFUNCTION(BlueprintPure, Category="Hex Sector|Generation") bool ValidateGeneratedLayout(FString& OutDiagnostic) const;

	UPROPERTY(BlueprintAssignable, Category="Hex Sector|Exploration")
	FSectorStateChangedSignature OnSectorStateChanged;

private:
	FIntPoint RoundAxial(const FVector2D& LocalPosition) const;
	FVector AxialToWorld(int32 Q, int32 R) const;
	FColor GetDebugColor(ESectorState State) const;
	void DrawGrid() const;
	void EnsureDefaultTemplates();
	void AssignTemplates(int32 Seed);
	FVector TransformTemplatePoint(const FHexSector& Sector, FVector2D Point) const;
};
