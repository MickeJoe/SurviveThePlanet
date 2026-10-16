#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"
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

	int32 GetMissionProgressReward() const
	{
		int64 Total = 0;
		for (const auto& Effect : CompletionBonus)
		{
			if (Effect.Type == ESTPContractEffectType::MissionProgress) Total += Effect.Amount;
		}
		return static_cast<int32>(FMath::Clamp<int64>(Total, 0, MAX_int32));
	}
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

UENUM(BlueprintType)
enum class ESTPContractState : uint8 { None, Active, Completed, Failed };

USTRUCT(BlueprintType)
struct FSTPContractRuntimeState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="Contracts") ESTPContractState State = ESTPContractState::None;
	UPROPERTY(BlueprintReadOnly, Category="Contracts") int32 DeliveriesMade = 0;
	UPROPERTY(BlueprintReadOnly, Category="Contracts") int32 DeliveriesMissed = 0;
	/** One simulation second equals one minute on the game clock. */
	UPROPERTY(BlueprintReadOnly, Category="Contracts") float SecondsUntilDelivery = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FContractStateChangedSignature);

/** Loads UI-ready contract offer definitions from Config/ContractOffers.json. */
UCLASS()
class SURVIVETHEPLANET_API UContractOfferSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	UFUNCTION(BlueprintPure, Category="Contracts") FSTPContractRuntimeState GetSelectedContractState() const { return RuntimeState; }
	UFUNCTION(BlueprintPure, Category="Contracts") bool CanDeliverSelectedContract() const;
	/** Pays for one shipment; completion rewards require every scheduled shipment. */
	UFUNCTION(BlueprintCallable, Category="Contracts") bool DeliverSelectedContract();
	UFUNCTION(BlueprintPure, Category="Contracts") int32 GetTraderReputation(FName TraderId) const { return TraderReputation.FindRef(TraderId); }
	UFUNCTION(BlueprintPure, Category="Contracts") float GetTraderCooldownSeconds(FName TraderId) const { return TraderCooldowns.FindRef(TraderId); }
	UPROPERTY(BlueprintAssignable, Category="Contracts") FContractStateChangedSignature OnContractStateChanged;
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
	bool bSettlingDelivery = false;
	FSTPContractRuntimeState RuntimeState;
	TMap<FName, int32> TraderReputation;
	TMap<FName, float> TraderCooldowns;
	bool ResolveSelectedContract(FSTPContractOfferDefinition& Offer, FSTPContractTierDefinition& Tier) const;
	AResourceManager* FindResourceManager() const;
	bool IsMissionActive() const;
	void SettleDelivery(bool bDelivered);
	int32 ApplyEffects(const TArray<FSTPContractEffectDefinition>& Effects, FName TraderId);
	void GrantBlueprintReward(const FSTPContractEffectDefinition& Effect);

	bool LoadFromJson(const FString& FilePath, TArray<FSTPContractOfferDefinition>& OutOffers) const;
};
