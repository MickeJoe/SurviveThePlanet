#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneSpawnPoint.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class APlanetSurfaceManager;

UENUM(BlueprintType)
enum class EDroneType : uint8
{
	Construction UMETA(DisplayName = "Construction"),
	Mining UMETA(DisplayName = "Mining"),
	Repair UMETA(DisplayName = "Repair"),
	Transport UMETA(DisplayName = "Transport"),
	Scout UMETA(DisplayName = "Scout"),
	Custom UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class ESpawnDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard UMETA(DisplayName = "Hard"),
	Expert UMETA(DisplayName = "Expert")
};

USTRUCT(BlueprintType)
struct FDroneSpawnOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	ESpawnDifficulty MinDifficulty = ESpawnDifficulty::Easy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	TSubclassOf<AActor> DroneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 SpawnCount = 1;
};

/**
 * Editor-placeable spawn point for drones. Place multiple instances in a level and
 * configure each one with its own drone type, class, mesh fallback, count, and radius.
 */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ADroneSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ADroneSpawnPoint();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:
	/** Spawn all drones configured for this point. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Drone Spawn")
	void SpawnDrones();

	/** Destroy drones spawned by this point. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Drone Spawn")
	void ClearSpawnedDrones();

	/** Returns the transform for a spawned drone by index. */
	UFUNCTION(BlueprintPure, Category = "Drone Spawn")
	FTransform GetSpawnTransform(int32 SpawnIndex) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> SpawnDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent;

	/** Gameplay type for this spawn point. Used for tags and quick filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	EDroneType DroneType = EDroneType::Construction;

	/** Optional label used when DroneType is Custom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn", meta = (EditCondition = "DroneType == EDroneType::Custom", EditConditionHides))
	FName CustomDroneType = TEXT("CustomDrone");

	/** Actor class to spawn for this drone point. Prefer setting this to a drone Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	TSubclassOf<AActor> DroneClass;

	/** Current difficulty used to choose a spawn option. Later this can be set by GameMode/GameInstance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	ESpawnDifficulty ActiveDifficulty = ESpawnDifficulty::Easy;

	/** Optional per-difficulty spawn options. If empty, DroneClass/fallback BaseDrone is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	TArray<FDroneSpawnOption> SpawnOptions;

	/** Optional mesh shown in the editor and used as fallback when DroneClass is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	TObjectPtr<UStaticMesh> DronePreviewMesh;

	/** Number of drones this point spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 SpawnCount = 1;

	/** Radius for spreading multiple spawned drones around this point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SpawnRadius = 150.0f;

	/** Spawn drones when play starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	bool bSpawnOnBeginPlay = true;

	/** Destroy this point's previous drones before spawning replacements. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	bool bReplaceExistingSpawns = true;

	/** Collision policy used for spawned drones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	/** Generic tag added to all spawned drone actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drone Spawn")
	FName DroneTag = TEXT("Drone");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone Spawn")
	TArray<TObjectPtr<AActor>> SpawnedDrones;

private:
	AActor* SpawnSingleDrone(int32 SpawnIndex);
	const FDroneSpawnOption* GetBestSpawnOption() const;
	UClass* GetEffectiveDroneClass() const;
	int32 GetEffectiveSpawnCount() const;
	UStaticMesh* GetEffectivePreviewMesh() const;
	FName GetDroneTypeTag() const;
	APlanetSurfaceManager* FindPlanetSurfaceManager() const;
	FIntPoint GetEffectiveDroneFootprint() const;
};
