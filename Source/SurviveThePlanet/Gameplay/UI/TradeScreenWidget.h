#pragma once
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Gameplay/Trading/TradeSubsystem.h"
#include "TradeScreenWidget.generated.h"
class UTradeItemWidget;
class UResourceDisplayWidget;
class UUniformGridPanel;
class UVerticalBox;
class UTextBlock;
class UImage;
class UButton;
UCLASS()

class SURVIVETHEPLANET_API UTradeScreenWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void OpenForTrader(FName Id);
	UFUNCTION(BlueprintCallable, Category = "Trading") void CloseTrade();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override;
	UPROPERTY(EditDefaultsOnly, Category = "Trading")
	TSubclassOf<UTradeItemWidget> ItemCardClass;
	UPROPERTY(EditDefaultsOnly, Category = "Trading")
	TSubclassOf<UTradeItemWidget> BasketRowClass;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> TraderGrid;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> PlayerGrid;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> BuyingList;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UVerticalBox> SellingList;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> TraderNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> TraderPortrait;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> CreditsText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> BuyingTotalText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> SellingTotalText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> BalanceText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NetCostText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ClearButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

private:
	TWeakObjectPtr<UResourceDisplayWidget> SimulationHUD;
	FName TraderId;
	TMap<FName, int32> Buying, Selling;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTradeItemWidget>> TraderCards;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTradeItemWidget>> PlayerCards;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTradeItemWidget>> BasketRows;
	TArray<FTradeItemDefinition> Items, StockItems;
	float RefreshElapsed = 0.0f;
	bool bInputLocked = false;
	FText LastResult;
	UTradeSubsystem* Trading() const;
	void ChangeQuantity(FName Id, bool bBuy, int32 Delta);
	void BuildCards();
	void RebuildBaskets();
	void Refresh();
	void RestorePlayerState();
	TArray<FTradeLine> Lines(const TMap<FName, int32>& Quantities) const;
	UFUNCTION()
	void Confirm();
	UFUNCTION()
	void Clear();
};
