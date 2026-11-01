#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Buildings/CargoBay.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
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
	MissionChoiceGroups.Reset();
	MissionChoiceStates.Reset();
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
	MissionChoiceGroups.Reset();
	MissionChoiceStates.Reset();
	CargoPackages.Reset();
	SelectedCargoPackage = NAME_None;
	MaxActiveObjectives = 32;
	NextActivationSequence = 0;
	MissionGameMinutes = 480.0;
	MissionDayLengthMinutes = 1440;

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
				else if (Type == TEXT("give_mission_progress"))
				{
					Reward.Type = ESTPObjectiveRewardType::GiveMissionProgress;
					bDefinitionValid = STPObjectives::ReadPositiveInt(RewardObject, TEXT("amount"), Reward.Amount);
				}
				else if (Type == TEXT("give_drone"))
				{
					FString DroneClassPath;
					Reward.Type = ESTPObjectiveRewardType::GiveDrone;
					bDefinitionValid = RewardObject->TryGetStringField(TEXT("droneClass"), DroneClassPath)
						&& !DroneClassPath.IsEmpty()
						&& STPObjectives::ReadPositiveInt(RewardObject, TEXT("amount"), Reward.Amount);
					Reward.DroneClass = FSoftClassPath(DroneClassPath);
				}
				else if (Type == TEXT("give_building_blueprint"))
				{
					FString BlueprintId;
					Reward.Type = ESTPObjectiveRewardType::GiveBuildingBlueprint;
					bDefinitionValid = RewardObject->TryGetStringField(TEXT("blueprintId"), BlueprintId)
						&& !BlueprintId.IsEmpty();
					Reward.BlueprintId = FName(*BlueprintId);
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

 const TArray<TSharedPtr<FJsonValue>>* Packages = nullptr;
 if (Root->TryGetArrayField(TEXT("cargoPackages"), Packages))
 {
  TSet<FName> PackageIds;
  for (const auto& Value : *Packages)
  {
   const auto Object = Value.IsValid() ? Value->AsObject() : nullptr;
   FString Id;
   const TArray<TSharedPtr<FJsonValue>>* Blueprints = nullptr;
   const TArray<TSharedPtr<FJsonValue>>* Objectives = nullptr;
   if (!Object.IsValid() || !Object->TryGetStringField(TEXT("id"), Id) || Id.IsEmpty()
    || PackageIds.Contains(FName(*Id))
    || !Object->TryGetArrayField(TEXT("blueprints"), Blueprints) || Blueprints->Num() != 3
    || !Object->TryGetArrayField(TEXT("objectives"), Objectives) || Objectives->Num() != 3)
   {
    UE_LOG(LogObjectives, Error, TEXT("Cargo packages require a unique id, three blueprints and three requests."));
    bValid = false;
    continue;
   }
   FCargoPackage Package;
   Package.Id = FName(*Id);
   for (const auto& Entry : *Blueprints)
   {
    FString Name;
    if (!Entry->TryGetString(Name) || Name.IsEmpty()) { bValid = false; continue; }
    Package.BlueprintIds.AddUnique(FName(*Name));
   }
   for (const auto& Entry : *Objectives)
   {
    FString Name;
    if (!Entry->TryGetString(Name) || !Definitions.Contains(FName(*Name))) { bValid = false; continue; }
    Package.ObjectiveIds.AddUnique(FName(*Name));
   }
   if (Package.BlueprintIds.Num() != 3 || Package.ObjectiveIds.Num() != 3) bValid = false;
   PackageIds.Add(Package.Id);
   CargoPackages.Add(MoveTemp(Package));
  }
 }

	const TArray<TSharedPtr<FJsonValue>>* ChoiceGroupValues = nullptr;
	if (Root->TryGetArrayField(TEXT("missionChoiceGroups"), ChoiceGroupValues))
	{
		for (const TSharedPtr<FJsonValue>& GroupValue : *ChoiceGroupValues)
		{
			const TSharedPtr<FJsonObject> GroupObject = GroupValue.IsValid() ? GroupValue->AsObject() : nullptr;
			FString GroupIdText, GroupTitle, GroupDescription;
			const TArray<TSharedPtr<FJsonValue>>* SlotValues = nullptr;
			if (!GroupObject.IsValid() || !GroupObject->TryGetStringField(TEXT("id"), GroupIdText) || GroupIdText.IsEmpty()
				|| !GroupObject->TryGetStringField(TEXT("title"), GroupTitle) || GroupTitle.IsEmpty()
				|| !GroupObject->TryGetArrayField(TEXT("slots"), SlotValues) || SlotValues->IsEmpty())
			{
				UE_LOG(LogObjectives, Error, TEXT("Each mission choice group needs id, title and non-empty slots."));
				bValid = false;
				continue;
			}
			GroupObject->TryGetStringField(TEXT("description"), GroupDescription);
			FSTPMissionChoiceGroupDefinition Group;
			Group.Id = FName(*GroupIdText);
			Group.Title = FText::FromString(GroupTitle);
			Group.Description = FText::FromString(GroupDescription);
			FString Prerequisite;
			GroupObject->TryGetStringField(TEXT("activateAfterObjective"), Prerequisite);
			Group.ActivateAfterObjective = FName(*Prerequisite);
			GroupObject->TryGetBoolField(TEXT("randomAssignment"), Group.bRandomAssignment);
			double UnlockDay = 1, UnlockMinute = 480;
			GroupObject->TryGetNumberField(TEXT("unlockDay"), UnlockDay);
			GroupObject->TryGetNumberField(TEXT("unlockMinuteOfDay"), UnlockMinute);
			if (MissionChoiceGroups.Num() >= 10 || UnlockDay < 1 || UnlockDay > 365
				|| UnlockMinute < 0 || UnlockMinute >= 1440
				|| UnlockDay != FMath::FloorToDouble(UnlockDay) || UnlockMinute != FMath::FloorToDouble(UnlockMinute))
			{
				UE_LOG(LogObjectives, Error, TEXT("Mission packets support up to 10 groups with valid unlock dates."));
				bValid = false; continue;
			}
			Group.UnlockDay = static_cast<int32>(UnlockDay);
			Group.UnlockMinuteOfDay = static_cast<int32>(UnlockMinute);
			if (!Group.ActivateAfterObjective.IsNone() && !Definitions.Contains(Group.ActivateAfterObjective))
			{
				bValid = false;
				continue;
			}
			if (MissionChoiceStates.Contains(Group.Id))
			{
				bValid = false;
				continue;
			}
			TSet<FName> GroupCandidates;
			bool bGroupValid = true;
			for (const TSharedPtr<FJsonValue>& SlotValue : *SlotValues)
			{
				const TSharedPtr<FJsonObject> SlotObject = SlotValue.IsValid() ? SlotValue->AsObject() : nullptr;
				FString SlotIdText, SlotTitle;
				const TArray<TSharedPtr<FJsonValue>>* CandidateValues = nullptr;
				if (!SlotObject.IsValid() || !SlotObject->TryGetStringField(TEXT("id"), SlotIdText) || SlotIdText.IsEmpty()
					|| !SlotObject->TryGetStringField(TEXT("title"), SlotTitle) || SlotTitle.IsEmpty()
					|| !SlotObject->TryGetArrayField(TEXT("objectives"), CandidateValues) || CandidateValues->IsEmpty())
				{
					bGroupValid = false;
					break;
				}
				FSTPMissionChoiceSlotDefinition Slot;
				Slot.Id = FName(*SlotIdText);
				Slot.Title = FText::FromString(SlotTitle);
				for (const TSharedPtr<FJsonValue>& CandidateValue : *CandidateValues)
				{
					FString CandidateText;
					const FName CandidateId = CandidateValue.IsValid() && CandidateValue->TryGetString(CandidateText)
						? FName(*CandidateText) : NAME_None;
					if (CandidateId.IsNone() || !Definitions.Contains(CandidateId) || GroupCandidates.Contains(CandidateId))
					{
						bGroupValid = false;
						break;
					}
					Slot.ObjectiveIds.Add(CandidateId);
					GroupCandidates.Add(CandidateId);
				}
				Group.Slots.Add(MoveTemp(Slot));
			}
			if (!bGroupValid)
			{
				UE_LOG(LogObjectives, Error, TEXT("Mission choice group '%s' has an invalid slot or objective."), *GroupIdText);
				bValid = false;
				continue;
			}
			MissionChoiceGroups.Add(Group);
			FSTPMissionChoiceGroupState& ChoiceState = MissionChoiceStates.Add(Group.Id);
			ChoiceState.SelectedObjectiveIds.Init(NAME_None, Group.Slots.Num());
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
	if (!State || !Definition || State->State != ESTPObjectiveState::Available || CountActiveObjectives() >= MaxActiveObjectives
		|| (IsMissionChoiceCandidate(ObjectiveId) && !IsConfirmedMissionChoice(ObjectiveId)))
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

bool UObjectiveSubsystem::GetDeliveryCosts(FName ObjectiveId, TArray<FResourceCost>& OutCosts) const
{
	OutCosts.Reset();
	const auto* State = RuntimeStates.Find(ObjectiveId);
	const auto* Definition = Definitions.Find(ObjectiveId);
	const auto* Mission = GetWorld() ? GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>() : nullptr;
	if (bDeliveringObjective || !Mission || Mission->GetMissionOutcome() != EMissionOutcome::InProgress
		|| !State || !Definition || State->State != ESTPObjectiveState::Active) return false;
	bool bCargoReady = false;
	for (TActorIterator<ACargoBay> It(GetWorld()); It; ++It)
		if (!It->IsPlacementPreview() && It->GetConstructionProgress() >= 1.0f) { bCargoReady = true; break; }
	if (!bCargoReady) return false;
	TMap<EResourceType, int32> Totals;
	for (int32 Index = 0; Index < Definition->Conditions.Num(); ++Index)
	{
		const auto& Condition = Definition->Conditions[Index];
		if (Condition.Type != ESTPObjectiveConditionType::DeliverResource || Condition.Destination != TEXT("HomeBase")
			|| !State->ConditionProgress.IsValidIndex(Index)) return false;
		const int32 Remaining = FMath::Max(0, Condition.RequiredAmount - State->ConditionProgress[Index]);
		int32& Total = Totals.FindOrAdd(Condition.ResourceType);
		if (Remaining > MAX_int32 - Total) return false;
		Total += Remaining;
	}
	for (const auto& Pair : Totals) if (Pair.Value > 0) OutCosts.Add({Pair.Key, Pair.Value});
	return !OutCosts.IsEmpty();
}

bool UObjectiveSubsystem::CanDeliverObjective(FName ObjectiveId) const
{
	TArray<FResourceCost> Costs;
	const auto* Resources = FindResourceManager();
	return Resources && GetDeliveryCosts(ObjectiveId, Costs) && Resources->CanAffordCosts(Costs);
}

bool UObjectiveSubsystem::DeliverObjective(FName ObjectiveId)
{
	TArray<FResourceCost> Costs;
	auto* Resources = FindResourceManager();
	if (!Resources || !GetDeliveryCosts(ObjectiveId, Costs)) return false;
	TGuardValue<bool> DeliveryGuard(bDeliveringObjective, true);
	if (!Resources->TrySpendCosts(Costs)) return false;
	auto& State = RuntimeStates.FindChecked(ObjectiveId);
	const auto& Definition = Definitions.FindChecked(ObjectiveId);
	for (int32 Index = 0; Index < Definition.Conditions.Num(); ++Index)
	{
		State.ConditionProgress[Index] = Definition.Conditions[Index].RequiredAmount;
		OnObjectiveProgressChanged.Broadcast(ObjectiveId, Index, State.ConditionProgress[Index], State.ConditionProgress[Index]);
	}
	CompleteObjective(ObjectiveId);
	return true;
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
	const auto* Mission = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>();
	if (Mission && Mission->GetMissionOutcome() != EMissionOutcome::InProgress) return;
	FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	const FSTPObjectiveDefinition* Definition = Definitions.Find(ObjectiveId);
	if (!State || !Definition || State->State != ESTPObjectiveState::Active)
	{
		return;
	}

	State->State = ESTPObjectiveState::Completed;
	State->RemainingTimeSeconds = -1.0f;
	OnObjectiveStateChanged.Broadcast(ObjectiveId, State->State);
 // Keep the choice for the entire run and grant the complete package atomically
 // through the same reward path as ordinary objectives.
 FSTPObjectiveDefinition Completion = *Definition;
 if (ObjectiveId == TEXT("mission_build_cargo_bay") && SelectedCargoPackage.IsNone() && !CargoPackages.IsEmpty())
 {
  const FCargoPackage& Package = CargoPackages[FMath::RandRange(0, CargoPackages.Num() - 1)];
  SelectedCargoPackage = Package.Id;
  for (FName Id : Package.BlueprintIds)
  {
   FSTPObjectiveRewardDefinition Reward;
   Reward.Type = ESTPObjectiveRewardType::GiveBuildingBlueprint;
   Reward.BlueprintId = Id;
   Completion.Rewards.Add(Reward);
  }
  UE_LOG(LogObjectives, Log, TEXT("Mother Base assigned Cargo package %s."), *SelectedCargoPackage.ToString());
 }
 GrantRewards(Completion, *State);
	// Assignment is permanent for this run; reopening the panel never rerolls it.
	for (const FSTPMissionChoiceGroupDefinition& Group : MissionChoiceGroups)
	{
		if (!Group.bRandomAssignment || Group.ActivateAfterObjective != ObjectiveId) continue;
		for (int32 SlotIndex = 0; SlotIndex < Group.Slots.Num(); ++SlotIndex)
		{
			const TArray<FName>& Candidates = Group.Slots[SlotIndex].ObjectiveIds;
			ConfirmMissionChoiceSlot(Group.Id, SlotIndex, Candidates[FMath::RandRange(0, Candidates.Num() - 1)]);
		}
	}
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
		else if (Reward.Type == ESTPObjectiveRewardType::GiveDrone)
		{
			for (int32 Index = 0; Index < Reward.Amount; ++Index)
			{
				if (!SpawnRewardDrone(Reward, Definition.Id))
				{
					break;
				}
			}
		}
		else if (Reward.Type == ESTPObjectiveRewardType::GiveBuildingBlueprint)
		{
			UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
			UBuildingBlueprintSubsystem* BlueprintInventory = GameInstance
				? GameInstance->GetSubsystem<UBuildingBlueprintSubsystem>() : nullptr;
			if (!BlueprintInventory || !BlueprintInventory->GrantBlueprintById(Reward.BlueprintId))
			{
				UE_LOG(LogObjectives, Warning, TEXT("Objective '%s' could not grant building blueprint '%s' (missing or already owned)."),
					*Definition.Id.ToString(), *Reward.BlueprintId.ToString());
			}
		}
	}
	// End the mission only after all other rewards in this completion have settled.
	if (const int32 ProgressReward = Definition.GetMissionProgressReward(); ProgressReward > 0)
	{
		if (auto* Mission = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>()) Mission->AddMissionProgress(ProgressReward);
	}
}

bool UObjectiveSubsystem::DebugCompleteObjective(FName ObjectiveId)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const FSTPObjectiveRuntimeState* State = RuntimeStates.Find(ObjectiveId);
	if (!State || State->State != ESTPObjectiveState::Active)
	{
		return false;
	}
	CompleteObjective(ObjectiveId);
	return true;
#endif
}

bool UObjectiveSubsystem::SpawnRewardDrone(const FSTPObjectiveRewardDefinition& Reward, FName SourceObjectiveId) const
{
	UWorld* World = GetWorld();
	UClass* DroneClass = Reward.DroneClass.TryLoadClass<AActor>();
	if (!World || !DroneClass || !DroneClass->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogObjectives, Error, TEXT("Objective '%s' could not load reward drone class '%s'."),
			*SourceObjectiveId.ToString(), *Reward.DroneClass.ToString());
		return false;
	}

	FVector SpawnLocation = FVector(0.0, 0.0, 250.0);
	FRotator SpawnRotation = FRotator::ZeroRotator;
	if (APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			SpawnLocation = Pawn->GetActorLocation() + Pawn->GetActorRightVector() * 350.0f + FVector(0.0f, 0.0f, 150.0f);
			SpawnRotation = Pawn->GetActorRotation();
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* SpawnedDrone = World->SpawnActor<AActor>(DroneClass, SpawnLocation, SpawnRotation, Params);
	if (!SpawnedDrone)
	{
		UE_LOG(LogObjectives, Error, TEXT("Objective '%s' failed to spawn reward drone '%s'."),
			*SourceObjectiveId.ToString(), *Reward.DroneClass.ToString());
		return false;
	}
	UE_LOG(LogObjectives, Log, TEXT("Objective '%s' granted drone '%s'."),
		*SourceObjectiveId.ToString(), *GetNameSafe(SpawnedDrone));
	return true;
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
		if (!IsMissionChoiceCandidate(Pair.Key) || IsConfirmedMissionChoice(Pair.Key))
		{
			ActivateObjective(Pair.Key);
		}
	}
}

TArray<FSTPMissionChoiceGroupDefinition> UObjectiveSubsystem::GetMissionChoiceGroups() const
{
	return MissionChoiceGroups;
}

double UObjectiveSubsystem::GetMissionUnlockMinutes(const FSTPMissionChoiceGroupDefinition& Group) const
{
	return static_cast<double>(Group.UnlockDay - 1) * MissionDayLengthMinutes + Group.UnlockMinuteOfDay;
}

void UObjectiveSubsystem::UpdateMissionClock(double TotalMinutes, int32 DayLengthMinutes)
{
	if (!FMath::IsFinite(TotalMinutes)) return;
	MissionGameMinutes = FMath::Max(0.0, TotalMinutes);
	MissionDayLengthMinutes = FMath::Max(1, DayLengthMinutes);
}

bool UObjectiveSubsystem::IsMissionChoiceGroupUnlocked(FName GroupId) const
{
	const auto* Group = MissionChoiceGroups.FindByPredicate([GroupId](const auto& Item) { return Item.Id == GroupId; });
	if (!Group || MissionGameMinutes < GetMissionUnlockMinutes(*Group)) return false;
	const auto* Prerequisite = RuntimeStates.Find(Group->ActivateAfterObjective);
	return Group->ActivateAfterObjective.IsNone() || (Prerequisite && Prerequisite->State == ESTPObjectiveState::Completed);
}

void UObjectiveSubsystem::ReconcileSavedObjectives()
{
	// Older slots do not contain newly introduced packet candidates or choice states.
	for (const auto& Pair : Definitions) if (!RuntimeStates.Contains(Pair.Key))
	{
		auto& State = RuntimeStates.Add(Pair.Key);
		State.ObjectiveId = Pair.Key;
		State.ConditionProgress.Init(0, Pair.Value.Conditions.Num());
	}
	// Automatic Mother Base requests were replaced by packet choices. Discard their
	// saved runtime records without completing them or granting their old rewards.
	for (const FName RetiredId : {FName(TEXT("mother_base_glass")), FName(TEXT("mother_base_polymer")),
		FName(TEXT("mother_base_water")), FName(TEXT("mother_base_fuel"))})
		RuntimeStates.Remove(RetiredId);
	for (const auto& Group : MissionChoiceGroups)
	{
		auto& State = MissionChoiceStates.FindOrAdd(Group.Id);
		State.SelectedObjectiveIds.SetNum(Group.Slots.Num());
	}
}

bool UObjectiveSubsystem::GetMissionChoiceGroupState(FName ChoiceGroupId, FSTPMissionChoiceGroupState& OutState) const
{
	if (const FSTPMissionChoiceGroupState* State = MissionChoiceStates.Find(ChoiceGroupId))
	{
		OutState = *State;
		return true;
	}
	return false;
}

bool UObjectiveSubsystem::ConfirmMissionChoices(FName ChoiceGroupId, const TArray<FName>& SelectedObjectiveIds)
{
	FSTPMissionChoiceGroupState* ChoiceState = MissionChoiceStates.Find(ChoiceGroupId);
	const FSTPMissionChoiceGroupDefinition* Group = MissionChoiceGroups.FindByPredicate(
		[ChoiceGroupId](const FSTPMissionChoiceGroupDefinition& Item) { return Item.Id == ChoiceGroupId; });
	if (!IsMissionChoiceGroupUnlocked(ChoiceGroupId)) return false;

	if (!ChoiceState || !Group || ChoiceState->bLocked || SelectedObjectiveIds.Num() != Group->Slots.Num()
		|| CountActiveObjectives() + SelectedObjectiveIds.Num() > MaxActiveObjectives)
	{
		return false;
	}
	for (int32 Index = 0; Index < Group->Slots.Num(); ++Index)
	{
		if (!Group->Slots[Index].ObjectiveIds.Contains(SelectedObjectiveIds[Index]))
		{
			return false;
		}
	}

	ChoiceState->SelectedObjectiveIds = SelectedObjectiveIds;
	for (const FName ObjectiveId : SelectedObjectiveIds)
	{
		MakeObjectiveAvailable(ObjectiveId);
		if (!ActivateObjective(ObjectiveId))
		{
			return false;
		}
	}
	ChoiceState->bLocked = true;
	OnMissionChoiceConfirmed.Broadcast(ChoiceGroupId);
	return true;
}

bool UObjectiveSubsystem::ConfirmMissionChoiceSlot(FName ChoiceGroupId, int32 SlotIndex, FName ObjectiveId)
{
	FSTPMissionChoiceGroupState* ChoiceState = MissionChoiceStates.Find(ChoiceGroupId);
	const FSTPMissionChoiceGroupDefinition* Group = MissionChoiceGroups.FindByPredicate(
		[ChoiceGroupId](const FSTPMissionChoiceGroupDefinition& Item) { return Item.Id == ChoiceGroupId; });
	if (!IsMissionChoiceGroupUnlocked(ChoiceGroupId)) return false;

	if (!ChoiceState || !Group || !Group->Slots.IsValidIndex(SlotIndex)
		|| !Group->Slots[SlotIndex].ObjectiveIds.Contains(ObjectiveId)
		|| CountActiveObjectives() >= MaxActiveObjectives)
	{
		return false;
	}

	ChoiceState->SelectedObjectiveIds.SetNum(Group->Slots.Num());
	if (!ChoiceState->SelectedObjectiveIds[SlotIndex].IsNone())
	{
		return false;
	}

	ChoiceState->SelectedObjectiveIds[SlotIndex] = ObjectiveId;
	MakeObjectiveAvailable(ObjectiveId);
	if (!ActivateObjective(ObjectiveId))
	{
		ChoiceState->SelectedObjectiveIds[SlotIndex] = NAME_None;
		return false;
	}
	ChoiceState->bLocked = !ChoiceState->SelectedObjectiveIds.Contains(NAME_None);
	OnMissionChoiceConfirmed.Broadcast(ChoiceGroupId);
	return true;
}

bool UObjectiveSubsystem::IsMissionChoiceCandidate(FName ObjectiveId) const
{
	for (const FSTPMissionChoiceGroupDefinition& Group : MissionChoiceGroups)
	{
		for (const FSTPMissionChoiceSlotDefinition& Slot : Group.Slots)
		{
			if (Slot.ObjectiveIds.Contains(ObjectiveId)) return true;
		}
	}
	return false;
}

bool UObjectiveSubsystem::IsConfirmedMissionChoice(FName ObjectiveId) const
{
	for (const TPair<FName, FSTPMissionChoiceGroupState>& Pair : MissionChoiceStates)
	{
		if (Pair.Value.SelectedObjectiveIds.Contains(ObjectiveId)) return true;
	}
	return false;
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
