#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Trading/TradeCatalog.h"
#include "Gameplay/Trading/MerchantDefinition.h"
#include "Subsystems/WorldSubsystem.h"
#include "TradeSubsystem.generated.h"

class ABaseDrone;
class AMerchantShip;
class ACargoBay;
class AResourceManager;

USTRUCT(BlueprintType)

struct FTradeLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading")
	FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading")
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)

struct FTradeQuote
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	int32 PurchaseTotal = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	int32 SaleTotal = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	int32 BalanceAfter = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	bool bCanConfirm = false;
	UPROPERTY(BlueprintReadOnly, Category = "Trading")
	FText Status;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTradeCompleted);

/** Validates and settles a whole basket against existing player inventories. */
UCLASS()

class SURVIVETHEPLANET_API UTradeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	UFUNCTION(BlueprintPure, Category = "Trading|Visits")
	FMerchantVisitState GetVisitState();
	UFUNCTION(BlueprintPure, Category = "Trading|Visits")
	UMerchantDefinition* GetMerchant(FName TraderId);
	UFUNCTION(BlueprintPure, Category = "Trading|Visits")
	bool IsTraderAvailable(FName TraderId);
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	UFUNCTION(BlueprintPure, Category = "Trading")
	UTradeCatalog* GetCatalog();
	UFUNCTION(BlueprintPure, Category = "Trading")
	TArray<FTradeItemDefinition> GetItems();
	UFUNCTION(BlueprintPure, Category = "Trading")
	int32 GetStock(FName TraderId, FName ItemId);
	UFUNCTION(BlueprintPure, Category = "Trading")
	int32 GetOwnedCount(FName ItemId);
	UFUNCTION(BlueprintPure, Category = "Trading")
	int32 GetSellableCount(FName ItemId);
	UFUNCTION(BlueprintPure, Category = "Trading")
	FTradeQuote QuoteTrade(FName TraderId, const TArray<FTradeLine>& Buying, const TArray<FTradeLine>& Selling);
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool ConfirmTrade(FName TraderId, const TArray<FTradeLine>& Buying, const TArray<FTradeLine>& Selling,
					  FText& OutStatus);
	UPROPERTY(BlueprintAssignable, Category = "Trading")
	FOnTradeCompleted OnTradeCompleted;

private:
	friend class FSTPMerchantVisitsTest;
	UPROPERTY(Transient)
	TObjectPtr<UTradeCatalog> Catalog;
	// Each trader keeps its own stock throughout the current world session.
	TMap<FName, TMap<FName, int32>> TraderStock;

	bool bCommitting = false;
	UPROPERTY(Transient)
	TObjectPtr<class UPlanetDefinition> Planet;
	FMerchantVisitState VisitState;
	double MinutesSinceCargoBay = 0.0;
	double ActiveArrivalMinute = -1.0;
	float CargoCheckElapsed = 0.0f;
	bool bCargoBayReady = false;
	UPROPERTY(Transient) TObjectPtr<AMerchantShip> MerchantShip;
	TWeakObjectPtr<ACargoBay> DockCargoBay;
	void UpdateMerchantShip();
	void RefreshVisitState();
	void ResolveCargoBay();
	const FTradeItemDefinition* FindItem(FName Id);
	AResourceManager* FindResourceManager() const;
	TArray<ABaseDrone*> FindDrones(const FTradeItemDefinition& Item, bool bSellableOnly) const;
	bool PrepareDroneDeliveries(const TArray<FTradeLine>& Buying, TArray<ABaseDrone*>& OutDrones);
	void RollbackDroneDeliveries(const TArray<ABaseDrone*>& Drones);
};
