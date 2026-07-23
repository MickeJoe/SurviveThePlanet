#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Drones/DroneSpawnPoint.h"
#include "BaseModuleSpawnPoint.generated.h"

class UArrowComponent;
class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class APlanetSurfaceManager;

USTRUCT(BlueprintType)
struct FBaseModuleSpawnOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	ESpawnDifficulty MinDifficulty = ESpawnDifficulty::Easy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	TSubclassOf<AActor> BaseModuleClass;
};

/**
 * Editor-placeable spawn point for a base module.
 */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ABaseModuleSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ABaseModuleSpawnPoint();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:
	/** Spawn the configured base module at this point. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Base Module")
	AActor* SpawnBaseModule();

	/** Destroy the last base module spawned by this point. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Base Module")
	void ClearSpawnedBaseModule();

	/** Transform used when spawning the base module. */
	UFUNCTION(BlueprintPure, Category = "Base Module")
	FTransform GetSpawnTransform() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> SpawnDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent;

	/** Actor class to spawn for the base module. Prefer setting this to a Base Module Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	TSubclassOf<AActor> BaseModuleClass;

	/** Current difficulty used to choose a spawn option. Later this can be set by GameMode/GameInstance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	ESpawnDifficulty ActiveDifficulty = ESpawnDifficulty::Easy;

	/** Optional per-difficulty spawn options. If empty, BaseModuleClass/fallback BaseBuilding is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	TArray<FBaseModuleSpawnOption> SpawnOptions;

	/** Optional mesh shown in the editor and used as fallback when BaseModuleClass is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	TObjectPtr<UStaticMesh> BaseModulePreviewMesh;

	/** Spawn the base module when play starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	bool bSpawnOnBeginPlay = true;

	/** Destroy this point's previous spawn before spawning a replacement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	bool bReplaceExistingSpawn = true;

	/** Collision policy used for spawned base modules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	/** Tag added to spawned base module actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Module")
	FName SpawnedActorTag = TEXT("BaseModule");

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Base Module")
	TObjectPtr<AActor> SpawnedBaseModule;

private:
	const FBaseModuleSpawnOption* GetBestSpawnOption() const;
	UClass* GetEffectiveBaseModuleClass() const;
	UStaticMesh* GetEffectivePreviewMesh() const;
	APlanetSurfaceManager* FindPlanetSurfaceManager() const;
	FIntPoint GetEffectiveBaseModuleFootprint() const;
};
