#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ColonySaveSubsystem.generated.h"

USTRUCT()
struct FColonySavedField
{
	GENERATED_BODY()
	UPROPERTY() FName Name;
	UPROPERTY() TArray<uint8> Data;
};

USTRUCT()
struct FColonySavedObject
{
	GENERATED_BODY()
	UPROPERTY() FString Id;
	UPROPERTY() FSoftClassPath Class;
	UPROPERTY() FTransform Transform;
	UPROPERTY() bool bTickEnabled = true;
	UPROPERTY() TArray<FColonySavedField> Fields;
};

USTRUCT(BlueprintType)
struct FColonySaveEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Slot;
	UPROPERTY(BlueprintReadOnly) FString DisplayName;
	UPROPERTY(BlueprintReadOnly) FDateTime SavedAt;
	UPROPERTY(BlueprintReadOnly) FString Map;
};

UCLASS()
class SURVIVETHEPLANET_API UColonySaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() float CameraZoom = 0.0f;
	UPROPERTY() FColonySaveEntry Entry;
	UPROPERTY() TArray<FColonySavedObject> Actors;
	UPROPERTY() TArray<FColonySavedObject> Systems;
	UPROPERTY() TArray<uint8> TraderStock;
};

/** Versioned colony snapshots. Slot IDs are independent of user-facing names. */
UCLASS()
class SURVIVETHEPLANET_API UColonySaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	UFUNCTION(BlueprintPure) FString GetSuggestedSaveName() const;
	UFUNCTION(BlueprintPure) bool CanSaveCurrentWorld() const;
	UFUNCTION(BlueprintCallable) TArray<FColonySaveEntry> ListSaves() const;
	UFUNCTION(BlueprintCallable) bool SaveColony(const FString& DisplayName, FText& Status);
	UFUNCTION(BlueprintCallable) bool LoadColony(const FString& Slot, FText& Status);
	UPROPERTY(BlueprintReadOnly) FText LastLoadStatus;
	// Shared with integration tests; does not travel to another map.
	UColonySaveGame* CaptureWorld(UWorld* World, FText& Status) const;
	bool RestoreWorld(UWorld* World, UColonySaveGame* Save, FText& Status);
private:
	UPROPERTY(Transient) TObjectPtr<UColonySaveGame> PendingLoad;
	FDelegateHandle MapLoadedHandle;
	void MapLoaded(UWorld* World);
	void FinishLoading(TWeakObjectPtr<UWorld> World, int32 Attempts);
};
