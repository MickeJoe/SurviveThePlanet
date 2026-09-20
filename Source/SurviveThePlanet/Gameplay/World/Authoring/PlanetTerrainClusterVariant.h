#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "PlanetTerrainClusterShapeActor.h"
#include "PlanetTerrainClusterVariant.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UPlanetSectorTemplate;
class AStaticMeshActor;

USTRUCT(BlueprintType)
struct FPlanetClusterElement
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform Transform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UMaterialInterface>> Materials;
};

/** Editable mesh composition in the compatible shape's local coordinate frame. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetTerrainClusterVariant : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName VariantId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UPlanetTerrainClusterShape> CompatibleShape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPlanetClusterElement> Elements;
	/** Reserved capabilities; POC generation never adds rotation or mirroring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSupportsRotation = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSupportsMirroring = false;
};

/** Explicit cookable catalog. Compatibility always comes from each variant's shape reference. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetTerrainClusterLibrary : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UPlanetTerrainClusterVariant>> Variants;
	TArray<UPlanetTerrainClusterVariant*> FindCompatible(const UPlanetTerrainClusterShape* Shape) const;
};

/** One actor per generated sector, with mesh/material groups rendered as ISM instances. */
UCLASS()
class SURVIVETHEPLANET_API APlanetGeneratedSector : public AActor
{
	GENERATED_BODY()
public:
	APlanetGeneratedSector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation") TObjectPtr<UPlanetSectorTemplate> SectorTemplate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation") TObjectPtr<UPlanetTerrainClusterLibrary> VariantLibrary;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation") int32 Seed = 12345;
	/** Population owns residency; editor previews still generate immediately. */
	UPROPERTY(Transient) bool bDeferRuntimeGeneration = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation|Performance") bool bOptimizeRendering = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Generation|Performance") bool bResident = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Generation") TArray<FName> SelectedVariantIds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Generation") TArray<FString> Diagnostics;
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Generation") void Generate();
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Generation") void ClearGenerated();
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedStaticMeshComponent>> InstanceGroups;
	void AddComposition(const UPlanetTerrainClusterVariant* Variant, const FTransform& SlotTransform);
};

/** Reuses the shape actor's footprint. MeshActors is the explicit authoring membership. */
UCLASS()
class SURVIVETHEPLANET_API APlanetTerrainClusterAuthoringActor : public APlanetTerrainClusterShapeActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Cluster Authoring") TObjectPtr<UPlanetTerrainClusterVariant> TargetClusterVariant;
	UPROPERTY(EditAnywhere, Category="Cluster Authoring") TArray<TObjectPtr<AStaticMeshActor>> MeshActors;
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cluster Authoring") void BakeClusterVariant();
#endif
};
