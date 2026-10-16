#include "Gameplay/Contracts/ContractOfferSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Trading/TraderSubsystem.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/BuildingCatalogDataAsset.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogContractOffers, Log, All);

namespace STPContracts
{
	bool ParseEffectType(const FString& Text, ESTPContractEffectType& OutType)
	{
		if (Text == TEXT("credits")) OutType = ESTPContractEffectType::Credits;
		else if (Text == TEXT("resource")) OutType = ESTPContractEffectType::Resource;
		else if (Text == TEXT("missionConfidence")) OutType = ESTPContractEffectType::MissionConfidence;
		else if (Text == TEXT("traderReputation")) OutType = ESTPContractEffectType::TraderReputation;
		else if (Text == TEXT("traderCooldownHours")) OutType = ESTPContractEffectType::TraderCooldownHours;
		else if (Text == TEXT("missionProgress")) OutType = ESTPContractEffectType::MissionProgress;
		else if (Text == TEXT("blueprint")) OutType = ESTPContractEffectType::Blueprint;
		else return false;
		return true;
	}

	bool ReadInteger(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& OutValue, bool bPositiveOnly)
	{
		double Number = 0.0;
		if (!Object.IsValid() || !Object->TryGetNumberField(Field, Number) || Number > MAX_int32 || Number < MIN_int32
			|| (bPositiveOnly && Number <= 0.0) || !FMath::IsNearlyEqual(Number, FMath::RoundToDouble(Number)))
		{
			return false;
		}
		OutValue = static_cast<int32>(Number);
		return true;
	}

	bool ParseAccentColor(const FString& Text, FLinearColor& OutColor)
	{
		if (Text.Len() != 6 && Text.Len() != 8)
		{
			return false;
		}
		for (const TCHAR Character : Text)
		{
			if (!FChar::IsHexDigit(Character))
			{
				return false;
			}
		}
		OutColor = FLinearColor::FromSRGBColor(FColor::FromHex(Text));
		return true;
	}

	bool ReadEffectArray(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Field,
		TArray<FSTPContractEffectDefinition>& OutEffects, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Parent->TryGetArrayField(Field, Values))
		{
			OutError = FString::Printf(TEXT("Missing '%s' array."), Field);
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
			FString TypeText, IdText, PoolIdText, IconReference;
			FSTPContractEffectDefinition Effect;
			if (!Object.IsValid() || !Object->TryGetStringField(TEXT("type"), TypeText)
				|| !ParseEffectType(TypeText, Effect.Type)
				|| !ReadInteger(Object, TEXT("amount"), Effect.Amount, false)
				|| Effect.Amount == 0
				|| !Object->TryGetStringField(TEXT("iconReference"), IconReference) || IconReference.IsEmpty())
			{
				OutError = FString::Printf(TEXT("Invalid effect in '%s'."), Field);
				return false;
			}

			Object->TryGetStringField(TEXT("id"), IdText);
			Object->TryGetStringField(TEXT("poolId"), PoolIdText);
			if ((Effect.Type == ESTPContractEffectType::Resource && IdText.IsEmpty())
				|| (Effect.Type == ESTPContractEffectType::Blueprint && PoolIdText.IsEmpty()))
			{
				OutError = FString::Printf(TEXT("Effect '%s' needs its associated id."), *TypeText);
				return false;
			}

			Effect.Id = FName(*IdText);
			Effect.PoolId = FName(*PoolIdText);
			Effect.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconReference));
			OutEffects.Add(MoveTemp(Effect));
		}
		return true;
	}
}

void UContractOfferSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadContractOffers();
}

void UContractOfferSubsystem::Deinitialize()
{
	ContractOffers.Reset();
	SelectedOfferId = NAME_None;
	SelectedTierId = NAME_None;
	bSelectionLocked = false;
	RuntimeState = FSTPContractRuntimeState();
	TraderReputation.Reset();
	TraderCooldowns.Reset();
	Super::Deinitialize();
}

bool UContractOfferSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UContractOfferSubsystem::ReloadContractOffers()
{
	const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("ContractOffers.json"));
	TArray<FSTPContractOfferDefinition> LoadedOffers;
	if (bSelectionLocked || !LoadFromJson(Path, LoadedOffers)) return false;
	ContractOffers = MoveTemp(LoadedOffers);
	UE_LOG(LogContractOffers, Log, TEXT("Loaded %d contract offers from %s."), ContractOffers.Num(), *Path);
	return true;
}

bool UContractOfferSubsystem::GetContractOffer(FName OfferId, FSTPContractOfferDefinition& OutOffer) const
{
	if (const FSTPContractOfferDefinition* Offer = ContractOffers.FindByPredicate(
		[OfferId](const FSTPContractOfferDefinition& Candidate) { return Candidate.Id == OfferId; }))
	{
		OutOffer = *Offer;
		return true;
	}
	return false;
}

TArray<FSTPContractOfferDefinition> UContractOfferSubsystem::GetContractOffersForTrader(FName TraderId) const
{
	TArray<FSTPContractOfferDefinition> Result;
	for (const FSTPContractOfferDefinition& Offer : ContractOffers)
	{
		if (Offer.TraderId == TraderId) Result.Add(Offer);
	}
	return Result;
}

bool UContractOfferSubsystem::ToggleContractSelection(FName OfferId, FName TierId)
{
	if (bSelectionLocked || bSettlingDelivery || !IsMissionActive())
	{
		return false;
	}
	const FSTPContractOfferDefinition* Offer = ContractOffers.FindByPredicate(
		[OfferId](const FSTPContractOfferDefinition& Candidate) { return Candidate.Id == OfferId; });
	if (!Offer || GetTraderCooldownSeconds(Offer->TraderId) > 0.0f || !Offer->Tiers.ContainsByPredicate(
		[TierId](const FSTPContractTierDefinition& Candidate) { return Candidate.Id == TierId; }))
	{
		return false;
	}

	if (SelectedOfferId == OfferId && SelectedTierId == TierId)
	{
		SelectedOfferId = NAME_None;
		SelectedTierId = NAME_None;
	}
	else
	{
		SelectedOfferId = OfferId;
		SelectedTierId = TierId;
	}
	return true;
}

bool UContractOfferSubsystem::IsContractTierSelected(FName OfferId, FName TierId) const
{
	return SelectedOfferId == OfferId && SelectedTierId == TierId;
}

bool UContractOfferSubsystem::GetSelectedContract(FName& OutOfferId, FName& OutTierId) const
{
	OutOfferId = SelectedOfferId;
	OutTierId = SelectedTierId;
	return !SelectedOfferId.IsNone() && !SelectedTierId.IsNone();
}

bool UContractOfferSubsystem::ConfirmSelectedContract()
{
	if (bSelectionLocked || bSettlingDelivery || SelectedOfferId.IsNone() || SelectedTierId.IsNone())
	{
		return false;
	}
	FSTPContractOfferDefinition Offer;
	FSTPContractTierDefinition Tier;
	if (!IsMissionActive() || !ResolveSelectedContract(Offer, Tier) || GetTraderCooldownSeconds(Offer.TraderId) > 0.0f) return false;
	bSelectionLocked = true;
	RuntimeState = FSTPContractRuntimeState();
	RuntimeState.State = ESTPContractState::Active;
	RuntimeState.SecondsUntilDelivery = Tier.DeliveryIntervalHours * 60.0f;
	OnContractStateChanged.Broadcast();
	return true;
}

bool UContractOfferSubsystem::LoadFromJson(const FString& FilePath,
	TArray<FSTPContractOfferDefinition>& OutOffers) const
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		UE_LOG(LogContractOffers, Error, TEXT("Could not read %s."), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogContractOffers, Error, TEXT("Invalid JSON in %s."), *FilePath);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* OfferValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("contractOffers"), OfferValues))
	{
		UE_LOG(LogContractOffers, Error, TEXT("ContractOffers.json needs a 'contractOffers' array."));
		return false;
	}

	TSet<FName> OfferIds;
	for (const TSharedPtr<FJsonValue>& OfferValue : *OfferValues)
	{
		const TSharedPtr<FJsonObject> Object = OfferValue.IsValid() ? OfferValue->AsObject() : nullptr;
		FString IdText, TraderIdText, ItemIdText, TitleText, IconReference;
		const TArray<TSharedPtr<FJsonValue>>* TierValues = nullptr;
		if (!Object.IsValid()
			|| !Object->TryGetStringField(TEXT("id"), IdText) || IdText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("traderId"), TraderIdText) || TraderIdText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("itemId"), ItemIdText) || ItemIdText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("title"), TitleText) || TitleText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("iconReference"), IconReference) || IconReference.IsEmpty()
			|| !Object->TryGetArrayField(TEXT("tiers"), TierValues) || TierValues->IsEmpty())
		{
			UE_LOG(LogContractOffers, Error, TEXT("Each offer needs id, traderId, itemId, title, iconReference and tiers."));
			return false;
		}

		const FName OfferId(*IdText);
		if (OfferIds.Contains(OfferId))
		{
			UE_LOG(LogContractOffers, Error, TEXT("Duplicate contract offer id '%s'."), *IdText);
			return false;
		}

		FSTPContractOfferDefinition Offer;
		Offer.Id = OfferId;
		Offer.TraderId = FName(*TraderIdText);
		Offer.ItemId = FName(*ItemIdText);
		if (StaticEnum<EResourceType>()->GetValueByNameString(ItemIdText) == INDEX_NONE)
		{
			UE_LOG(LogContractOffers, Error, TEXT("Contract '%s' itemId '%s' is not an inventory resource."), *IdText, *ItemIdText);
			return false;
		}
		Offer.Title = FText::FromString(TitleText);
		Offer.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconReference));
		TSet<FName> TierIds;

		for (const TSharedPtr<FJsonValue>& TierValue : *TierValues)
		{
			const TSharedPtr<FJsonObject> TierObject = TierValue.IsValid() ? TierValue->AsObject() : nullptr;
			FString TierIdText, DisplayNameText, AccentColorText, Error;
			FSTPContractTierDefinition Tier;
			int32 IntervalHours = 0;
			if (!TierObject.IsValid()
				|| !TierObject->TryGetStringField(TEXT("id"), TierIdText) || TierIdText.IsEmpty()
				|| !TierObject->TryGetStringField(TEXT("displayName"), DisplayNameText) || DisplayNameText.IsEmpty()
				|| !TierObject->TryGetStringField(TEXT("accentColor"), AccentColorText)
				|| !STPContracts::ReadInteger(TierObject, TEXT("deliveryAmount"), Tier.DeliveryAmount, true)
				|| !STPContracts::ReadInteger(TierObject, TEXT("deliveryCount"), Tier.DeliveryCount, true)
				|| !STPContracts::ReadInteger(TierObject, TEXT("deliveryIntervalHours"), IntervalHours, true)
				|| !STPContracts::ParseAccentColor(AccentColorText, Tier.AccentColor)
				|| !STPContracts::ReadEffectArray(TierObject, TEXT("deliveryRewards"), Tier.DeliveryRewards, Error)
				|| !STPContracts::ReadEffectArray(TierObject, TEXT("missedDeliveryPenalties"), Tier.MissedDeliveryPenalties, Error)
				|| !STPContracts::ReadEffectArray(TierObject, TEXT("completionBonus"), Tier.CompletionBonus, Error))
			{
				UE_LOG(LogContractOffers, Error, TEXT("Invalid tier in offer '%s': %s"), *IdText, *Error);
				return false;
			}

			Tier.Id = FName(*TierIdText);
			if (TierIds.Contains(Tier.Id))
			{
				UE_LOG(LogContractOffers, Error, TEXT("Duplicate tier '%s' in offer '%s'."), *TierIdText, *IdText);
				return false;
			}
			Tier.DisplayName = FText::FromString(DisplayNameText);
			Tier.DeliveryIntervalHours = static_cast<float>(IntervalHours);
			TierIds.Add(Tier.Id);
			Offer.Tiers.Add(MoveTemp(Tier));
		}

		OfferIds.Add(OfferId);
		OutOffers.Add(MoveTemp(Offer));
	}
	return true;
}

TStatId UContractOfferSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UContractOfferSubsystem, STATGROUP_Tickables);
}

bool UContractOfferSubsystem::IsMissionActive() const
{
	const auto* Mission = GetWorld() ? GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>() : nullptr;
	return Mission && Mission->GetMissionOutcome() == EMissionOutcome::InProgress;
}

AResourceManager* UContractOfferSubsystem::FindResourceManager() const
{
	for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It) return *It;
	return nullptr;
}

bool UContractOfferSubsystem::ResolveSelectedContract(FSTPContractOfferDefinition& Offer, FSTPContractTierDefinition& Tier) const
{
	if (!GetContractOffer(SelectedOfferId, Offer)) return false;
	const auto* SelectedTier = Offer.Tiers.FindByPredicate([this](const auto& Candidate) { return Candidate.Id == SelectedTierId; });
	if (!SelectedTier) return false;
	Tier = *SelectedTier;
	return true;
}

bool UContractOfferSubsystem::CanDeliverSelectedContract() const
{
	if (bSettlingDelivery || !IsMissionActive() || RuntimeState.State != ESTPContractState::Active) return false;
	FSTPContractOfferDefinition Offer;
	FSTPContractTierDefinition Tier;
	if (!ResolveSelectedContract(Offer, Tier)) return false;
	const int64 Resource = StaticEnum<EResourceType>()->GetValueByNameString(Offer.ItemId.ToString());
	const AResourceManager* Manager = FindResourceManager();
	return Manager && Resource != INDEX_NONE && Manager->HasResource(static_cast<EResourceType>(Resource), Tier.DeliveryAmount);
}

bool UContractOfferSubsystem::DeliverSelectedContract()
{
	if (!CanDeliverSelectedContract()) return false;
	TGuardValue<bool> SettlementGuard(bSettlingDelivery, true);
	FSTPContractOfferDefinition Offer;
	FSTPContractTierDefinition Tier;
	if (!ResolveSelectedContract(Offer, Tier)) return false;
	const auto Resource = static_cast<EResourceType>(StaticEnum<EResourceType>()->GetValueByNameString(Offer.ItemId.ToString()));
	AResourceManager* Manager = FindResourceManager();
	if (!Manager || !Manager->TrySpendResource(Resource, Tier.DeliveryAmount)) return false;
	SettleDelivery(true);
	return true;
}

void UContractOfferSubsystem::Tick(float DeltaTime)
{
	if (bSettlingDelivery || !IsMissionActive() || DeltaTime <= 0.0f || UGameplayStatics::IsGamePaused(this)
		|| UGameplayStatics::GetGlobalTimeDilation(this) < 0.001f) return;
	for (auto& Cooldown : TraderCooldowns) Cooldown.Value = FMath::Max(0.0f, Cooldown.Value - DeltaTime);
	if (RuntimeState.State != ESTPContractState::Active) return;
	float RemainingDelta = DeltaTime;
	while (RuntimeState.State == ESTPContractState::Active && RemainingDelta >= RuntimeState.SecondsUntilDelivery && IsMissionActive())
	{
		RemainingDelta -= RuntimeState.SecondsUntilDelivery;
		if (!DeliverSelectedContract())
		{
			TGuardValue<bool> SettlementGuard(bSettlingDelivery, true);
			SettleDelivery(false);
		}
	}
	if (RuntimeState.State == ESTPContractState::Active) RuntimeState.SecondsUntilDelivery -= RemainingDelta;
}

void UContractOfferSubsystem::SettleDelivery(bool bDelivered)
{
	if (RuntimeState.State != ESTPContractState::Active) return;
	FSTPContractOfferDefinition Offer;
	FSTPContractTierDefinition Tier;
	if (!ResolveSelectedContract(Offer, Tier)) return;
	if (bDelivered) ++RuntimeState.DeliveriesMade;
	else ++RuntimeState.DeliveriesMissed;
	RuntimeState.SecondsUntilDelivery = Tier.DeliveryIntervalHours * 60.0f;
	const bool bAllShipmentsResolved = RuntimeState.DeliveriesMade + RuntimeState.DeliveriesMissed >= Tier.DeliveryCount;
	if (bAllShipmentsResolved)
	{
		RuntimeState.State = RuntimeState.DeliveriesMissed == 0 ? ESTPContractState::Completed : ESTPContractState::Failed;
		bSelectionLocked = false;
	}
	int64 ProgressReward = ApplyEffects(bDelivered ? Tier.DeliveryRewards : Tier.MissedDeliveryPenalties, Offer.TraderId);
	if (RuntimeState.State == ESTPContractState::Completed) ProgressReward += ApplyEffects(Tier.CompletionBonus, Offer.TraderId);
	OnContractStateChanged.Broadcast();
	// The terminal state above prevents duplicate completion rewards, including callback re-entry.
	if (auto* Mission = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>())
	{
		Mission->AddMissionProgress(static_cast<int32>(FMath::Clamp<int64>(ProgressReward, MIN_int32, MAX_int32)));
	}
}

int32 UContractOfferSubsystem::ApplyEffects(const TArray<FSTPContractEffectDefinition>& Effects, FName TraderId)
{
	int64 ProgressReward = 0;
	AResourceManager* Manager = FindResourceManager();
	for (const auto& Effect : Effects)
	{
		switch (Effect.Type)
		{
		case ESTPContractEffectType::MissionProgress: ProgressReward += Effect.Amount; break;
		case ESTPContractEffectType::Credits:
			if (Manager) Manager->AddCredits(Effect.Amount);
			break;
		case ESTPContractEffectType::Resource:
		{
			const int64 Resource = StaticEnum<EResourceType>()->GetValueByNameString(Effect.Id.ToString());
			if (Manager && Resource != INDEX_NONE) Manager->AddResource(static_cast<EResourceType>(Resource), Effect.Amount);
			break;
		}
		case ESTPContractEffectType::MissionConfidence:
			if (auto* Mission = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>()) Mission->AddMissionConfidence(Effect.Amount);
			break;
		case ESTPContractEffectType::TraderReputation:
			TraderReputation.FindOrAdd(TraderId) = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(GetTraderReputation(TraderId)) + Effect.Amount, MIN_int32, MAX_int32));
			break;
		case ESTPContractEffectType::TraderCooldownHours:
			TraderCooldowns.FindOrAdd(TraderId) = FMath::Max(GetTraderCooldownSeconds(TraderId), Effect.Amount * 60.0f);
			break;
		case ESTPContractEffectType::Blueprint: GrantBlueprintReward(Effect); break;
		}
	}
	return static_cast<int32>(FMath::Clamp<int64>(ProgressReward, MIN_int32, MAX_int32));
}

void UContractOfferSubsystem::GrantBlueprintReward(const FSTPContractEffectDefinition& Effect)
{
	const auto* Buildings = GetWorld()->GetSubsystem<UBuildingManagerSubsystem>();
	const auto* Catalog = Buildings ? Buildings->GetCatalog() : nullptr;
	auto* Inventory = GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>() : nullptr;
	if (!Catalog || !Inventory || Effect.PoolId != TEXT("rare_industrial_blueprints")) return;
	int32 Remaining = Effect.Amount;
	for (const auto& Definition : Catalog->Buildings)
	{
		if (Remaining <= 0) break;
		if (Definition && Definition->BuildCategory == ESTPBuildCategory::Industry && !Inventory->OwnsBlueprint(Definition->BuildTool)
			&& Inventory->GrantBlueprint(Definition->BuildTool)) --Remaining;
	}
}
