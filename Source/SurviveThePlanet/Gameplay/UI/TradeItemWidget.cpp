#include "Gameplay/UI/TradeItemWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"

void UTradeItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (PlusButton)
	{
		PlusButton->OnClicked.AddUniqueDynamic(this, &UTradeItemWidget::Increase);
	}
	if (MinusButton)
	{
		MinusButton->OnClicked.AddUniqueDynamic(this, &UTradeItemWidget::Decrease);
	}
	if (RemoveButton)
	{
		RemoveButton->OnClicked.AddUniqueDynamic(this, &UTradeItemWidget::Remove);
	}
}

void UTradeItemWidget::Configure(const FTradeItemDefinition& Item, bool bBuying)
{
	ItemId = Item.Id;
	bBuy = bBuying;
	ItemIcon->SetBrushFromTexture(Item.Icon);
	PriceText->SetText(FText::AsNumber(bBuy ? Item.BuyPrice : Item.SellPrice));
	if (ItemNameText)
	{
		ItemNameText->SetText(Item.DisplayName);
	}
	FString Tip = Item.DisplayName.ToString() + TEXT("\n") + Item.Description.ToString();
	Tip += FString::Printf(TEXT("\nBuy: %d  |  Sell: %d"), Item.BuyPrice, Item.SellPrice);
	if (Item.Kind == ETradeItemKind::Blueprint)
	{
		Tip += TEXT("\nLearned permanently. Cannot be sold.");
	}
	if (Item.Kind == ETradeItemKind::Drone && !bBuy)
	{
		Tip += TEXT("\nOnly idle drones can be sold.");
	}
	Tip += TEXT("\n+ / -: one item. Shift: 10. Ctrl: 100.");
	SetToolTipText(FText::FromString(Tip));
}

void UTradeItemWidget::UpdateCounts(int32 Available, int32 Selected, bool bSelectable)
{
	StockText->SetText(FText::AsNumber(Available));
	QuantityText->SetText(FText::AsNumber(Selected));
	PlusButton->SetIsEnabled(bSelectable && Selected < Available);
	MinusButton->SetIsEnabled(Selected > 0);
	if (RemoveButton)
	{
		RemoveButton->SetIsEnabled(Selected > 0);
	}
	ItemIcon->SetRenderOpacity(Available > 0 ? 1.f : .3f);
	if (SelectionBorder)
	{
		SelectionBorder->SetBrushColor(Selected > 0 ? FLinearColor(.022f, .15f, .17f, 1.f)
													: FLinearColor(.012f, .025f, .032f, 1.f));
	}
}

namespace
{
int32 Step()
{
	const auto Keys = FSlateApplication::Get().GetModifierKeys();
	return Keys.IsControlDown() ? 100 : Keys.IsShiftDown() ? 10 : 1;
}
} // namespace

void UTradeItemWidget::Increase()
{
	OnQuantityChanged.Broadcast(ItemId, bBuy, Step());
}

void UTradeItemWidget::Decrease()
{
	OnQuantityChanged.Broadcast(ItemId, bBuy, -Step());
}

void UTradeItemWidget::Remove()
{
	OnQuantityChanged.Broadcast(ItemId, bBuy, MIN_int32);
}
