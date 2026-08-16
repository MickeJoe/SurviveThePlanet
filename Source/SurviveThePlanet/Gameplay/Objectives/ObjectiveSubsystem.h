#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectiveSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESTPObjectiveState : uint8
{
	Locked,
	Available,
	Active,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class ESTPObjectiveConditionType : uint8
{
	BuildBuilding,
	DeliverResource
};

UENUM(BlueprintType)
enum class ESTPObjectiveRewardType : uint8
{
	GiveResource,
	UnlockObjective,
	GiveMissionConfidence
};

USTRUCT(BlueprintType)
struct FSTPObjectiveConditionDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	ESTPObjectiveConditionType Type = ESTPObjectiveConditionType::BuildBuilding;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	ESTPBuildingType BuildingType = ESTPBuildingType::Other;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	EResourceType ResourceType = EResourceType::Iron;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FName Destination;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	int32 RequiredAmount = 1;
};

USTRUCT(BlueprintType)
struct FSTPObjectiveRewardDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	ESTPObjectiveRewardType Type = ESTPObjectiveRewardType::GiveResource;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	EResourceType ResourceType = EResourceType::Iron;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FName ObjectiveId;
};

USTRUCT(BlueprintType)
struct FSTPObjectiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FText Description;

	/** Zero means unlimited. The countdown begins when the objective activates. */
	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	float TimeLimitSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	TArray<FSTPObjectiveConditionDefinition> Conditions;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	TArray<FSTPObjectiveRewardDefinition> Rewards;
};

USTRUCT(BlueprintType)
struct FSTPObjectiveRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	FName ObjectiveId;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	ESTPObjectiveState State = ESTPObjectiveState::Locked;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	TArray<int32> ConditionProgress;

	/** Negative when the objective is not timed. */
	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	float RemainingTimeSeconds = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	bool bRewardsGranted = false;

	/** Stable order used by the HUD; assigned when the objective activates. */
	UPROPERTY(BlueprintReadOnly, Category = "Objectives")
	int32 ActivationSequence = INDEX_NONE;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSTPObjectiveStateChangedSignature, FName, ObjectiveId, ESTPObjectiveState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSTPObjectiveProgressChangedSignature, FName, ObjectiveId, int32, ConditionIndex, int32, CurrentAmount, int32, RequiredAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSTPObjectiveTimeChangedSignature, FName, ObjectiveId, float, RemainingSeconds);

/** JSON-driven objective engine. Gameplay reports facts; this owns state, timers and rewards. */
UCLASS()
class SURVIVETHEPLANET_API UObjectiveSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/** Reloads Config/Objectives.json and resets runtime objective state. */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool ReloadObjectives();

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool ActivateObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives|Events")
	void SubmitBuildingBuilt(ESTPBuildingType BuildingType, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Objectives|Events")
	void SubmitResourceDelivered(EResourceType ResourceType, FName Destination, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FSTPObjectiveRuntimeState> GetActiveObjectives() const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetObjectiveDefinition(FName ObjectiveId, FSTPObjectiveDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetObjectiveState(FName ObjectiveId, FSTPObjectiveRuntimeState& OutState) const;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FSTPObjectiveStateChangedSignature OnObjectiveStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FSTPObjectiveProgressChangedSignature OnObjectiveProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FSTPObjectiveTimeChangedSignature OnObjectiveTimeChanged;

private:
	TMap<FName, FSTPObjectiveDefinition> Definitions;
	TMap<FName, FSTPObjectiveRuntimeState> RuntimeStates;
	TArray<FName> InitialObjectiveIds;
	int32 MaxActiveObjectives = 8;
	int32 NextActivationSequence = 0;

	bool LoadFromJson(const FString& FilePath);
	void ApplyProgressEvent(ESTPObjectiveConditionType EventType, ESTPBuildingType BuildingType,
		EResourceType ResourceType, FName Destination, int32 Amount);
	void CompleteObjective(FName ObjectiveId);
	void FailObjective(FName ObjectiveId);
	void GrantRewards(const FSTPObjectiveDefinition& Definition, FSTPObjectiveRuntimeState& State);
	void MakeObjectiveAvailable(FName ObjectiveId);
	void ActivateAvailableObjectives();
	int32 CountActiveObjectives() const;
	AResourceManager* FindResourceManager() const;
};
