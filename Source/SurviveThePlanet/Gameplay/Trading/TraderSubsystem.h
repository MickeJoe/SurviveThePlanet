#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "TraderSubsystem.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FSTPTraderGoodDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	EResourceType Resource = EResourceType::Iron;

	/** Production required before this trader may offer a contract for the resource. */
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	int32 ProductionThreshold = 0;
};

USTRUCT(BlueprintType)
struct FSTPTraderDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	FText Description;

	/** Soft reference to a Texture2D, for example /Game/UI/Traders/T_Ares.T_Ares. */
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	TSoftObjectPtr<UTexture2D> Icon;

	/** How long a newly unlocked contract offer remains available. */
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	float OfferDurationHours = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	TArray<FSTPTraderGoodDefinition> Goods;
};

/** Loads trader definitions from Config/Traders.json and exposes them to C++ and WBP. */
UCLASS()
class SURVIVETHEPLANET_API UTraderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/** Reloads Config/Traders.json. Existing data is kept if the new file is invalid. */
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool ReloadTraders();

	UFUNCTION(BlueprintPure, Category = "Trading")
	TArray<FSTPTraderDefinition> GetTraders() const { return Traders; }

	UFUNCTION(BlueprintPure, Category = "Trading")
	bool GetTrader(FName TraderId, FSTPTraderDefinition& OutTrader) const;

private:
	UPROPERTY()
	TArray<FSTPTraderDefinition> Traders;

	bool LoadFromJson(const FString& FilePath, TArray<FSTPTraderDefinition>& OutTraders) const;
};
