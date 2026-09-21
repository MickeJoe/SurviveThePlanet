#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/World/PlanetResourcePlacement.h"
#include "SectorPopulation.generated.h"

class ABaseResourceSource;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UPlanetSectorTemplate;
class UPlanetTerrainClusterLibrary;
class APlanetGeneratedSector;

USTRUCT(BlueprintType)
struct FSectorMeshMaterialSet
{
	GENERATED_BODY()
	/** Material-slot overrides matching the corresponding DecorationMeshes entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UMaterialInterface>> Materials;
};

USTRUCT(BlueprintType)
struct FSectorDecoration
{
	GENERATED_BODY()
	UPROPERTY(SaveGame, BlueprintReadOnly) int32 SectorId = 0;
	UPROPERTY(SaveGame, BlueprintReadOnly) int32 MeshIndex = 0;
	UPROPERTY(SaveGame, BlueprintReadOnly) FTransform Transform;
	UPROPERTY(SaveGame, BlueprintReadOnly) float Radius = 0.0f;
};

/** Place once in a map to connect authored layout, resource placement and discovery. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ASectorPopulation : public AActor
{
	GENERATED_BODY()
public:
	ASectorPopulation();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") TObjectPtr<AHexSectorGrid> Grid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") int32 Seed = 71237;
	/** Assign both to replace legacy dressing with authored cluster compositions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Authored Clusters") TObjectPtr<UPlanetSectorTemplate> AuthoredSectorTemplate;
	/** Optional catalog. When nonempty, replaces the single-template setting above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Authored Clusters") TArray<TObjectPtr<UPlanetSectorTemplate>> AuthoredSectorTemplates;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Authored Clusters") TObjectPtr<UPlanetTerrainClusterLibrary> ClusterVariantLibrary;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Population|Authored Clusters") TMap<int32, TObjectPtr<APlanetGeneratedSector>> GeneratedClusters;
	/** Only visual dressing is streamed. Discovery, deposits and simulation stay loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Performance") bool bManageClusterResidency = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Performance") bool bOptimizeClusterRendering = true;
	UPROPERTY(EditAnywhere, Category="Population|Performance", meta=(ClampMin="0")) float ClusterPrefetchMargin = 2500.0f;
	UPROPERTY(EditAnywhere, Category="Population|Performance", meta=(ClampMin="0")) float ClusterUnloadDelay = 3.0f;
	UPROPERTY(EditAnywhere, Category="Population|Performance", meta=(ClampMin="1")) int32 ClusterLoadsPerUpdate = 2;
	UFUNCTION(BlueprintCallable, Category="Population|Performance") void UpdateClusterResidency();
	UFUNCTION(BlueprintPure, Category="Population|Authored Clusters") UPlanetSectorTemplate* SelectTemplateForSector(int32 SectorId) const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") int32 DecorationsPerSector = 48;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population", meta=(ClampMin="1")) int32 FormationCountPerSector = 4;
	/** Fraction of a sector's ground covered by environment formation footprints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Coverage", meta=(ClampMin="0.0", ClampMax="0.75")) float TerrainCoverageTarget = 0.40f;
	/** Deterministic per-sector deviation around TerrainCoverageTarget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Coverage", meta=(ClampMin="0.0", ClampMax="0.25")) float TerrainCoverageVariation = 0.08f;
	/** Start sector is kept a little more open for initial construction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Coverage", meta=(ClampMin="0.5", ClampMax="1.0")) float StartSectorCoverageScale = 0.85f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population|Coverage", meta=(ClampMin="12", ClampMax="64")) int32 CoverageSamplesPerAxis = 32;
	/** DecorationMeshes begins with this many landmark meshes, followed by RockMeshCount rocks, then plants/ground cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population", meta=(ClampMin="1")) int32 LandmarkMeshCount = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population", meta=(ClampMin="1")) int32 RockMeshCount = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") TArray<TObjectPtr<UStaticMesh>> DecorationMeshes;
	/** Optional per-slot material overrides, parallel to DecorationMeshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") TArray<FSectorMeshMaterialSet> DecorationMaterialSets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population") TMap<EResourceType, TSubclassOf<ABaseResourceSource>> DepositClasses;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Population") FPlanetResourcePlacementResult Resources;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category="Population") TArray<FSectorDecoration> Decorations;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Population") TArray<FString> Diagnostics;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Population|Coverage") TMap<int32, float> ActualTerrainCoverage;
	UFUNCTION(BlueprintCallable, Category="Population") void InitializePopulation();
	UFUNCTION(BlueprintCallable, Category="Population") void RevealSector(int32 SectorId);
private:
	UFUNCTION() void OnSectorChanged(int32 SectorId, ESectorState State);
	bool GroundPosition(FVector Position, FVector& Ground) const;
	bool GenerateAuthoredClusters();
	TArray<UPlanetSectorTemplate*> GetAuthoredTemplateCatalog() const;
	bool GenerateLegacyDecorations();
	UPROPERTY(Transient) TSet<int32> LoadedSectors;
	UPROPERTY(Transient) TArray<TObjectPtr<ABaseResourceSource>> SpawnedDeposits;
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> InstanceGroups;
	bool bInitialized = false;
	double NextClusterResidencyUpdate = 0.0;
	TMap<int32, double> ClusterLastNeededTime;
};
