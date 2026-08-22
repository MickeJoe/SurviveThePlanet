#include "Gameplay/Contracts/ContractOfferSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Trading/TraderSubsystem.h"
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
	if (!LoadFromJson(Path, LoadedOffers)) return false;
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
	if (bSelectionLocked)
	{
		return false;
	}
	const FSTPContractOfferDefinition* Offer = ContractOffers.FindByPredicate(
		[OfferId](const FSTPContractOfferDefinition& Candidate) { return Candidate.Id == OfferId; });
	if (!Offer || !Offer->Tiers.ContainsByPredicate(
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
	if (bSelectionLocked || SelectedOfferId.IsNone() || SelectedTierId.IsNone())
	{
		return false;
	}
	bSelectionLocked = true;
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
