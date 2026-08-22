#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ContractOfferSubsystem.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ESTPContractEffectType : uint8
{
	Credits,
	Resource,
	MissionConfidence,
	TraderReputation,
	TraderCooldownHours,
	MissionProgress,
	Blueprint
};

USTRUCT(BlueprintType)
struct FSTPContractEffectDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	ESTPContractEffectType Type = ESTPContractEffectType::Credits;

	/** Resource id when Type is Resource. */
	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName Id;

	/** Blueprint pool id when Type is Blueprint. */
	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName PoolId;

	/** Signed value. Penalties commonly use a negative value; cooldown hours are positive. */
	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct FSTPContractTierDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FLinearColor AccentColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	int32 DeliveryAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	int32 DeliveryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	float DeliveryIntervalHours = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TArray<FSTPContractEffectDefinition> DeliveryRewards;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TArray<FSTPContractEffectDefinition> MissedDeliveryPenalties;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TArray<FSTPContractEffectDefinition> CompletionBonus;
};

USTRUCT(BlueprintType)
struct FSTPContractOfferDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName Id;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName TraderId;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "Contracts")
	TArray<FSTPContractTierDefinition> Tiers;
};

/** Loads UI-ready contract offer definitions from Config/ContractOffers.json. */
UCLASS()
class SURVIVETHEPLANET_API UContractOfferSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintCallable, Category = "Contracts")
	bool ReloadContractOffers();

	UFUNCTION(BlueprintPure, Category = "Contracts")
	TArray<FSTPContractOfferDefinition> GetContractOffers() const { return ContractOffers; }

	UFUNCTION(BlueprintPure, Category = "Contracts")
	bool GetContractOffer(FName OfferId, FSTPContractOfferDefinition& OutOffer) const;

	UFUNCTION(BlueprintPure, Category = "Contracts")
	TArray<FSTPContractOfferDefinition> GetContractOffersForTrader(FName TraderId) const;

	/** Selects this tier, or clears the selection when the same tier is selected again. */
	UFUNCTION(BlueprintCallable, Category = "Contracts")
	bool ToggleContractSelection(FName OfferId, FName TierId);

	UFUNCTION(BlueprintPure, Category = "Contracts")
	bool IsContractTierSelected(FName OfferId, FName TierId) const;

	UFUNCTION(BlueprintPure, Category = "Contracts")
	bool GetSelectedContract(FName& OutOfferId, FName& OutTierId) const;

	/** Locks the current selection. A locked contract can no longer be changed. */
	UFUNCTION(BlueprintCallable, Category = "Contracts")
	bool ConfirmSelectedContract();

	UFUNCTION(BlueprintPure, Category = "Contracts")
	bool IsContractSelectionLocked() const { return bSelectionLocked; }

private:
	UPROPERTY()
	TArray<FSTPContractOfferDefinition> ContractOffers;
	FName SelectedOfferId;
	FName SelectedTierId;
	bool bSelectionLocked = false;

	bool LoadFromJson(const FString& FilePath, TArray<FSTPContractOfferDefinition>& OutOffers) const;
};
