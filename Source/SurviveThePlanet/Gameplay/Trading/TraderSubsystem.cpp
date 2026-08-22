#include "Gameplay/Trading/TraderSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTraders, Log, All);

namespace STPTraders
{
	bool ParseResource(const FString& Text, EResourceType& OutResource)
	{
		const UEnum* Enum = StaticEnum<EResourceType>();
		const int64 Value = Enum ? Enum->GetValueByNameString(Text) : INDEX_NONE;
		if (Value == INDEX_NONE)
		{
			return false;
		}
		OutResource = static_cast<EResourceType>(Value);
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

void UTraderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadTraders();
}

void UTraderSubsystem::Deinitialize()
{
	Traders.Reset();
	Super::Deinitialize();
}

bool UTraderSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UTraderSubsystem::ReloadTraders()
{
	const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Traders.json"));
	TArray<FSTPTraderDefinition> LoadedTraders;
	if (!LoadFromJson(Path, LoadedTraders))
	{
		return false;
	}

	Traders = MoveTemp(LoadedTraders);
	UE_LOG(LogTraders, Log, TEXT("Loaded %d traders from %s."), Traders.Num(), *Path);
	return true;
}

bool UTraderSubsystem::GetTrader(FName TraderId, FSTPTraderDefinition& OutTrader) const
{
	if (const FSTPTraderDefinition* Trader = Traders.FindByPredicate(
		[TraderId](const FSTPTraderDefinition& Candidate) { return Candidate.Id == TraderId; }))
	{
		OutTrader = *Trader;
		return true;
	}
	return false;
}

bool UTraderSubsystem::LoadFromJson(const FString& FilePath, TArray<FSTPTraderDefinition>& OutTraders) const
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		UE_LOG(LogTraders, Error, TEXT("Could not read %s."), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTraders, Error, TEXT("Invalid JSON in %s."), *FilePath);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* TraderValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("traders"), TraderValues))
	{
		UE_LOG(LogTraders, Error, TEXT("Traders.json needs a 'traders' array."));
		return false;
	}

	TSet<FName> SeenIds;
	for (const TSharedPtr<FJsonValue>& TraderValue : *TraderValues)
	{
		const TSharedPtr<FJsonObject> Object = TraderValue.IsValid() ? TraderValue->AsObject() : nullptr;
		FString IdText, NameText, DescriptionText, IconReference;
		double OfferDurationHours = 0.0;
		const TArray<TSharedPtr<FJsonValue>>* GoodValues = nullptr;
		if (!Object.IsValid()
			|| !Object->TryGetStringField(TEXT("id"), IdText) || IdText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("name"), NameText) || NameText.IsEmpty()
			|| !Object->TryGetStringField(TEXT("description"), DescriptionText)
			|| !Object->TryGetStringField(TEXT("iconReference"), IconReference) || IconReference.IsEmpty()
			|| !Object->TryGetNumberField(TEXT("offerDurationHours"), OfferDurationHours) || OfferDurationHours <= 0.0
			|| !Object->TryGetArrayField(TEXT("goods"), GoodValues) || GoodValues->IsEmpty())
		{
			UE_LOG(LogTraders, Error, TEXT("Each trader needs id, name, description, iconReference and non-empty goods."));
			return false;
		}

		const FName TraderId(*IdText);
		if (SeenIds.Contains(TraderId))
		{
			UE_LOG(LogTraders, Error, TEXT("Duplicate trader id '%s'."), *IdText);
			return false;
		}

		FSTPTraderDefinition Trader;
		Trader.Id = TraderId;
		Trader.Name = FText::FromString(NameText);
		Trader.Description = FText::FromString(DescriptionText);
		Trader.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconReference));
		Trader.OfferDurationHours = static_cast<float>(OfferDurationHours);

		TSet<EResourceType> SeenResources;
		for (const TSharedPtr<FJsonValue>& GoodValue : *GoodValues)
		{
			const TSharedPtr<FJsonObject> GoodObject = GoodValue.IsValid() ? GoodValue->AsObject() : nullptr;
			FString ResourceText;
			FSTPTraderGoodDefinition Good;
			if (!GoodObject.IsValid()
				|| !GoodObject->TryGetStringField(TEXT("resource"), ResourceText)
				|| !STPTraders::ParseResource(ResourceText, Good.Resource)
				|| !STPTraders::ReadPositiveInt(GoodObject, TEXT("productionThreshold"), Good.ProductionThreshold)
				|| SeenResources.Contains(Good.Resource))
			{
				UE_LOG(LogTraders, Error, TEXT("Trader '%s' has an invalid or duplicate good."), *IdText);
				return false;
			}
			SeenResources.Add(Good.Resource);
			Trader.Goods.Add(Good);
		}

		SeenIds.Add(TraderId);
		OutTraders.Add(MoveTemp(Trader));
	}

	return true;
}
