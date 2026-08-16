#include "Gameplay/Objectives/ObjectiveSubsystem.h"

#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogObjectives, Log, All);

namespace STPObjectives
{
	template <typename TEnum>
	bool ParseEnum(const FString& Text, TEnum& OutValue)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		const int64 Value = Enum ? Enum->GetValueByNameString(Text) : INDEX_NONE;
		if (Value == INDEX_NONE)
		{
			return false;
		}
		OutValue = static_cast<TEnum>(Value);
		return true;
	}

	bool ReadPositiveInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& OutValue)
	{
		double Number = 0.0;
		if (!Object.IsValid() || !Object->TryGetNumberField(Field, Number) || Number < 1.0 || Number > MAX_int32
			|| !FMath::IsNearlyEqual(Number, FMath::RoundToDouble(Number)))
		{
			return false;
		}
		OutValue = static_cast<int32>(Number);
		return true;
	}
}

void UObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadObjectives();
}

void UObjectiveSubsystem::Deinitialize()
{
	Definitions.Reset();
	RuntimeStates.Reset();
	InitialObjectiveIds.Reset();
	Super::Deinitialize();
}

bool UObjectiveSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UObjectiveSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UObjectiveSubsystem, STATGROUP_Tickables);
}

void UObjectiveSubsystem::Tick(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	TArray<FName> Expired;
	for (TPair<FName, FSTPObjectiveRuntimeState>& Pair : RuntimeStates)
	{
		FSTPObjectiveRuntimeState& State = Pair.Value;
		if (State.State != ESTPObjectiveState::Active || State.RemainingTimeSeconds < 0.0f)
		{
			continue;
		}

		const int32 PreviousSecond = FMath::CeilToInt(State.RemainingTimeSeconds);
		State.RemainingTimeSeconds = FMath::Max(0.0f, State.RemainingTimeSeconds - DeltaTime);
		if (FMath::CeilToInt(State.RemainingTimeSeconds) != PreviousSecond)
		{
			OnObjectiveTimeChanged.Broadcast(Pair.Key, State.RemainingTimeSeconds);
		}
		if (State.RemainingTimeSeconds <= 0.0f)
		{
			Expired.Add(Pair.Key);
		}
	}

	for (const FName Id : Expired)
	{
		FailObjective(Id);
	}
}

bool UObjectiveSubsystem::ReloadObjectives()
{
	Definitions.Reset();
	RuntimeStates.Reset();
	InitialObjectiveIds.Reset();
	MaxActiveObjectives = 8;
	NextActivationSequence = 0;

	const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Objectives.json"));
	if (!LoadFromJson(Path))
	{
		return false;
	}

	for (const TPair<FName, FSTPObjectiveDefinition>& Pair : Definitions)
	{
		FSTPObjectiveRuntimeState& State = RuntimeStates.Add(Pair.Key);
		State.ObjectiveId = Pair.Key;
		State.ConditionProgress.Init(0, Pair.Value.Conditions.Num());
	}
	for (const FName Id : InitialObjectiveIds)
	{
		MakeObjectiveAvailable(Id);
	}
	ActivateAvailableObjectives();

	UE_LOG(LogObjectives, Log, TEXT("Loaded %d objectives (%d initial, max %d active) from %s."),
		Definitions.Num(), InitialObjectiveIds.Num(), MaxActiveObjectives, *Path);
	return true;
}

bool UObjectiveSubsystem::LoadFromJson(const FString& FilePath)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		UE_LOG(LogObjectives, Error, TEXT("Could not read %s."), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogObjectives, Error, TEXT("Invalid JSON in %s."), *FilePath);
		return false;
	}

	double Maximum = 8.0;
	if (Root->TryGetNumberField(TEXT("maxActiveObjectives"), Maximum))
	{
		MaxActiveObjectives = FMath::Clamp(FMath::FloorToInt(Maximum), 1, 32);
	}

	const TArray<TSharedPtr<FJsonValue>>* InitialValues = nullptr;
	if (Root->TryGetArrayField(TEXT("initialObjectives"), InitialValues))
	{
		for (const TSharedPtr<FJsonValue>& Value : *InitialValues)
		{
			FString Id;
			if (Value.IsValid() && Value->TryGetString(Id) && !Id.IsEmpty())
			{
				InitialObjectiveIds.AddUnique(FName(*Id));
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ObjectiveValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("objectives"), ObjectiveValues))
	{
		UE_LOG(LogObjectives, Error, TEXT("Objectives.json needs an 'objectives' array."));
		return false;
	}

	bool bValid = true;
	for (const TSharedPtr<FJsonValue>& Value : *ObjectiveValues)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		FString IdText;
		FString TitleText;
		if (!Object.IsValid() || !Object->TryGetStringField(TEXT("id"), IdText) || IdText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("title"), TitleText) || TitleText.IsEmpty())
		{
			UE_LOG(LogObjectives, Error, TEXT("Each objective needs non-empty id and title fields."));
			bValid = false;
			continue;
		}

		const FName Id(*IdText);
		if (Definitions.Contains(Id))
		{
			UE_LOG(LogObjectives, Error, TEXT("Duplicate objective id '%s'."), *IdText);
			bValid = false;
			continue;
		}

		FSTPObjectiveDefinition Definition;
		Definition.Id = Id;
		Definition.Title = FText::FromString(TitleText);
		FString Description;
		Object->TryGetStringField(TEXT("description"), Description);
		Definition.Description = FText::FromString(Description);
		double TimeLimit = 0.0;
		Object->TryGetNumberField(TEXT("timeLimitSeconds"), TimeLimit);
		Definition.TimeLimitSeconds = static_cast<float>(FMath::Max(0.0, TimeLimit));

		const TArray<TSharedPtr<FJsonValue>>* Conditions = nullptr;
		bool bDefinitionValid = Object->TryGetArrayField(TEXT("conditions"), Conditions) && !Conditions->IsEmpty();
		if (bDefinitionValid)
		{
			for (const TSharedPtr<FJsonValue>& ConditionValue : *Conditions)
			{
				const TSharedPtr<FJsonObject> ConditionObject = ConditionValue.IsValid() ? ConditionValue->AsObject() : nullptr;
				FString Type;
				FSTPObjectiveConditionDefinition Condition;
				if (!ConditionObject.IsValid() || !ConditionObject->TryGetStringField(TEXT("type"), Type)
					|| !STPObjectives::ReadPositiveInt(ConditionObject, TEXT("amount"), Condition.RequiredAmount))
				{
					bDefinitionValid = false;
					break;
				}

				if (Type == TEXT("build_building"))
				{
					FString Building;
					Condition.Type = ESTPObjectiveConditionType::BuildBuilding;
					bDefinitionValid = ConditionObject->TryGetStringField(TEXT("building"), Building)
						&& STPObjectives::ParseEnum(Building, Condition.BuildingType);
				}
				else if (Type == TEXT("deliver_resource"))
				{
					FString Resource;
					FString Destination;
					Condition.Type = ESTPObjectiveConditionType::DeliverResource;
					bDefinitionValid = ConditionObject->TryGetStringField(TEXT("resource"), Resource)
						&& STPObjectives::ParseEnum(Resource, Condition.ResourceType)
						&& ConditionObject->TryGetStringField(TEXT("destination"), Destination) && !Destination.IsEmpty();
					Condition.Destination = FName(*Destination);
				}
				else
				{
					bDefinitionValid = false;
				}

				if (!bDefinitionValid)
				{
					break;
				}
				Definition.Conditions.Add(Condition);
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* Rewards = nullptr;
		if (bDefinitionValid && Object->TryGetArrayField(TEXT("rewards"), Rewards))
		{
			for (const TSharedPtr<FJsonValue>& RewardValue : *Rewards)
			{
				const TSharedPtr<FJsonObject> RewardObject = RewardValue.IsValid() ? RewardValue->AsObject() : nullptr;
				FString Type;
				FSTPObjectiveRewardDefinition Reward;
				if (!RewardObject.IsValid() || !RewardObject->TryGetStringField(TEXT("type"), Type))
				{
					bDefinitionValid = false;
					break;
				}

				if (Type == TEXT("give_resource"))
				{
					FString Resource;
					Reward.Type = ESTPObjectiveRewardType::GiveResource;
					bDefinitionValid = RewardObject->TryGetStringField(TEXT("resource"), Resource)
						&& STPObjectives::ParseEnum(Resource, Reward.ResourceType)
						&& STPObjectives::ReadPositiveInt(RewardObject, TEXT("amount"), Reward.Amount);
				}
				else if (Type == TEXT("unlock_objective"))
				{
					FString UnlockedId;
					Reward.Type = ESTPObjectiveRewardType::UnlockObjective;
					bDefinitionValid = RewardObject->TryGetStringField(TEXT("objective"), UnlockedId) && !UnlockedId.IsEmpty();
					Reward.ObjectiveId = FName(*UnlockedId);
				}
				else if (Type == TEXT("give_mission_confidence"))
				{
					Reward.Type = ESTPObjectiveRewardType::GiveMissionConfidence;
					bDefinitionValid = STPObjectives::ReadPositiveInt(RewardObject, TEXT("amount"), Reward.Amount)
						&& Reward.Amount <= 100;
				}
				else
				{
					bDefinitionValid = false;
				}

				if (!bDefinitionValid)
				{
					break;
				}
				Definition.Rewards.Add(Reward);
			}
		}

		if (!bDefinitionValid)
		{
			UE_LOG(LogObjectives, Error, TEXT("Objective '%s' has an invalid condition or reward."), *IdText);
			bValid = false;
			continue;
		}
		Definitions.Add(Id, MoveTemp(Definition));
	}

	for (const FName Id : InitialObjectiveIds)
	{
		if (!Definitions.Contains(Id))
		{
			UE_LOG(LogObjectives, Error, TEXT("Initial objective '%s' does not exist."), *Id.ToString());
			bValid = false;
		}
	}
	for (const TPair<FName, FSTPObjectiveDefinition>& Pair : Definitions)
	{
		for (const FSTPObjectiveRewardDefinition& Reward : Pair.Value.Rewards)
		{
			if (Reward.Type == ESTPObjectiveRewardType::UnlockObjective && !Definitions.Contains(Reward.ObjectiveId))
			{
				UE_LOG(LogObjectives, Error, TEXT("'%s' unlocks missing objective '%s'."),
					*Pair.Key.ToString(), *Reward.ObjectiveId.ToString());
				bValid = false;
			}
		}
	}

	if (!bValid)
	{
		Definitions.Reset();
	}
	return bValid;
}

bool UObjectiveSubsystem::ActivateObjective(FName ObjectiveId)
{
	FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	const FSTPObjectiveDefinition* Definition = Definitions.Find(ObjectiveId);
	if (!State || !Definition || State->State != ESTPObjectiveState::Available || CountActiveObjectives() >= MaxActiveObjectives)
	{
		return false;
	}

	State->State = ESTPObjectiveState::Active;
	State->ActivationSequence = NextActivationSequence++;
	State->RemainingTimeSeconds = Definition->TimeLimitSeconds > 0.0f ? Definition->TimeLimitSeconds : -1.0f;
	OnObjectiveStateChanged.Broadcast(ObjectiveId, State->State);
	if (State->RemainingTimeSeconds >= 0.0f)
	{
		OnObjectiveTimeChanged.Broadcast(ObjectiveId, State->RemainingTimeSeconds);
	}
	return true;
}

void UObjectiveSubsystem::SubmitBuildingBuilt(ESTPBuildingType BuildingType, int32 Amount)
{
	ApplyProgressEvent(ESTPObjectiveConditionType::BuildBuilding, BuildingType, EResourceType::Iron, NAME_None, Amount);
}

void UObjectiveSubsystem::SubmitResourceDelivered(EResourceType ResourceType, FName Destination, int32 Amount)
{
	ApplyProgressEvent(ESTPObjectiveConditionType::DeliverResource, ESTPBuildingType::Other, ResourceType, Destination, Amount);
}

void UObjectiveSubsystem::ApplyProgressEvent(ESTPObjectiveConditionType EventType, ESTPBuildingType BuildingType,
	EResourceType ResourceType, FName Destination, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	TArray<FName> Completed;
	for (TPair<FName, FSTPObjectiveRuntimeState>& Pair : RuntimeStates)
	{
		FSTPObjectiveRuntimeState& State = Pair.Value;
		const FSTPObjectiveDefinition* Definition = Definitions.Find(Pair.Key);
		if (State.State != ESTPObjectiveState::Active || !Definition)
		{
			continue;
		}

		for (int32 Index = 0; Index < Definition->Conditions.Num(); ++Index)
		{
			const FSTPObjectiveConditionDefinition& Condition = Definition->Conditions[Index];
			const bool bMatches = Condition.Type == EventType
				&& (EventType != ESTPObjectiveConditionType::BuildBuilding || Condition.BuildingType == BuildingType)
				&& (EventType != ESTPObjectiveConditionType::DeliverResource
					|| (Condition.ResourceType == ResourceType && Condition.Destination == Destination));
			if (!bMatches)
			{
				continue;
			}

			int32& Progress = State.ConditionProgress[Index];
			Progress = FMath::Min(Condition.RequiredAmount, Progress + Amount);
			OnObjectiveProgressChanged.Broadcast(Pair.Key, Index, Progress, Condition.RequiredAmount);
		}

		bool bAllComplete = true;
		for (int32 Index = 0; Index < Definition->Conditions.Num(); ++Index)
		{
			bAllComplete &= State.ConditionProgress[Index] >= Definition->Conditions[Index].RequiredAmount;
		}
		if (bAllComplete)
		{
			Completed.Add(Pair.Key);
		}
	}

	for (const FName Id : Completed)
	{
		CompleteObjective(Id);
	}
}

void UObjectiveSubsystem::CompleteObjective(FName ObjectiveId)
{
	FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	const FSTPObjectiveDefinition* Definition = Definitions.Find(ObjectiveId);
	if (!State || !Definition || State->State != ESTPObjectiveState::Active)
	{
		return;
	}

	State->State = ESTPObjectiveState::Completed;
	State->RemainingTimeSeconds = -1.0f;
	OnObjectiveStateChanged.Broadcast(ObjectiveId, State->State);
	GrantRewards(*Definition, *State);
	ActivateAvailableObjectives();
}

void UObjectiveSubsystem::FailObjective(FName ObjectiveId)
{
	FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	if (!State || State->State != ESTPObjectiveState::Active)
	{
		return;
	}
	State->State = ESTPObjectiveState::Failed;
	State->RemainingTimeSeconds = 0.0f;
	OnObjectiveStateChanged.Broadcast(ObjectiveId, State->State);
	ActivateAvailableObjectives();
}

void UObjectiveSubsystem::GrantRewards(const FSTPObjectiveDefinition& Definition, FSTPObjectiveRuntimeState& State)
{
	if (State.bRewardsGranted)
	{
		return;
	}
	State.bRewardsGranted = true;

	for (const FSTPObjectiveRewardDefinition& Reward : Definition.Rewards)
	{
		if (Reward.Type == ESTPObjectiveRewardType::GiveResource)
		{
			if (AResourceManager* Manager = FindResourceManager())
			{
				Manager->AddResource(Reward.ResourceType, Reward.Amount);
			}
			else
			{
				UE_LOG(LogObjectives, Warning, TEXT("No ResourceManager found for reward from '%s'."), *Definition.Id.ToString());
			}
		}
		else if (Reward.Type == ESTPObjectiveRewardType::UnlockObjective)
		{
			MakeObjectiveAvailable(Reward.ObjectiveId);
		}
		else if (Reward.Type == ESTPObjectiveRewardType::GiveMissionConfidence)
		{
			if (UMissionConfidenceSubsystem* Confidence = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>())
			{
				Confidence->AddMissionConfidence(static_cast<float>(Reward.Amount));
			}
		}
	}
}

void UObjectiveSubsystem::MakeObjectiveAvailable(FName ObjectiveId)
{
	FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	if (State && State->State == ESTPObjectiveState::Locked)
	{
		State->State = ESTPObjectiveState::Available;
		OnObjectiveStateChanged.Broadcast(ObjectiveId, State->State);
	}
}

void UObjectiveSubsystem::ActivateAvailableObjectives()
{
	for (const FName InitialId : InitialObjectiveIds)
	{
		if (CountActiveObjectives() >= MaxActiveObjectives)
		{
			return;
		}
		ActivateObjective(InitialId);
	}

	for (TPair<FName, FSTPObjectiveRuntimeState>& Pair : RuntimeStates)
	{
		if (CountActiveObjectives() >= MaxActiveObjectives)
		{
			return;
		}
		ActivateObjective(Pair.Key);
	}
}

int32 UObjectiveSubsystem::CountActiveObjectives() const
{
	int32 Count = 0;
	for (const TPair<FName, FSTPObjectiveRuntimeState>& Pair : RuntimeStates)
	{
		Count += Pair.Value.State == ESTPObjectiveState::Active ? 1 : 0;
	}
	return Count;
}

TArray<FSTPObjectiveRuntimeState> UObjectiveSubsystem::GetActiveObjectives() const
{
	TArray<FSTPObjectiveRuntimeState> Result;
	for (const TPair<FName, FSTPObjectiveRuntimeState>& Pair : RuntimeStates)
	{
		if (Pair.Value.State == ESTPObjectiveState::Active)
		{
			Result.Add(Pair.Value);
		}
	}
	Result.Sort([](const FSTPObjectiveRuntimeState& A, const FSTPObjectiveRuntimeState& B)
	{
		return A.ActivationSequence < B.ActivationSequence;
	});
	return Result;
}

bool UObjectiveSubsystem::GetObjectiveDefinition(FName ObjectiveId, FSTPObjectiveDefinition& OutDefinition) const
{
	if (const FSTPObjectiveDefinition* Definition = Definitions.Find(ObjectiveId))
	{
		OutDefinition = *Definition;
		return true;
	}
	return false;
}

bool UObjectiveSubsystem::GetObjectiveState(FName ObjectiveId, FSTPObjectiveRuntimeState& OutState) const
{
	if (const FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId))
	{
		OutState = *State;
		return true;
	}
	return false;
}

AResourceManager* UObjectiveSubsystem::FindResourceManager() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}
