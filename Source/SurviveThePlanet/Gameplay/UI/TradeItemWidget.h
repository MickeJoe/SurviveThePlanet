#pragma once
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Gameplay/Trading/TradeCatalog.h"
#include "TradeItemWidget.generated.h"
class UButton;
class UImage;
class UTextBlock;
class UBorder;
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTradeQuantityChanged, FName, bool, int32);
/** Logic shared by the compact WBP card and WBP basket row. */
UCLASS()

class SURVIVETHEPLANET_API UTradeItemWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Configure(const FTradeItemDefinition& Item, bool bBuying);
	void UpdateCounts(int32 Available, int32 Selected, bool bSelectable);

	FName GetItemId() const
	{
		return ItemId;
	}

	bool IsBuying() const
	{
		return bBuy;
	}

	FTradeQuantityChanged OnQuantityChanged;

protected:
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> StockText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> PlusButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> MinusButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionBorder;

private:
	FName ItemId;
	bool bBuy = true;
	UFUNCTION()
	void Increase();
	UFUNCTION()
	void Decrease();
	UFUNCTION()
	void Remove();
};
