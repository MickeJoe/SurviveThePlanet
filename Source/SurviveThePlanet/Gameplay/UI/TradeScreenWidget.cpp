#include "Gameplay/UI/TradeScreenWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Trading/TraderSubsystem.h"
#include "Gameplay/UI/TradeItemWidget.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

UTradeSubsystem* UTradeScreenWidget::Trading() const
{
	return GetWorld()->GetSubsystem<UTradeSubsystem>();
}

void UTradeScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &UTradeScreenWidget::Confirm);
	ClearButton->OnClicked.AddUniqueDynamic(this, &UTradeScreenWidget::Clear);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UTradeScreenWidget::CloseTrade);
	SetIsFocusable(true);
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UResourceDisplayWidget::StaticClass(), false);
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget->GetOwningPlayer() == GetOwningPlayer())
		{
			SimulationHUD = CastChecked<UResourceDisplayWidget>(Widget);
			SimulationHUD->SetTradingPaused(true);
			break;
		}
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		bInputLocked = true;
		PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(TakeWidget()));
		PC->bShowMouseCursor = true;
	}
}

void UTradeScreenWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	// UI must keep updating when colony simulation is paused.
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.25f)
	{
		RefreshElapsed = 0.0f;
		Refresh();
	}
}

void UTradeScreenWidget::RestorePlayerState()
{

	if (bInputLocked)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
			PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
		}
		bInputLocked = false;
	}
	if (SimulationHUD.IsValid()) SimulationHUD->SetTradingPaused(false);
	SimulationHUD.Reset();
}

void UTradeScreenWidget::NativeDestruct()
{
	RestorePlayerState();
	Super::NativeDestruct();
}

FReply UTradeScreenWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape || KeyEvent.GetKey() == EKeys::I)
	{
		CloseTrade();
		return FReply::Handled();
	}
	return FReply::Handled();
}

void UTradeScreenWidget::OpenForTrader(FName Id)
{
	TraderId = Id;
	Buying.Empty();
	Selling.Empty();
	LastResult = FText::GetEmpty();

	if (const UMerchantDefinition* Merchant = Trading()->GetMerchant(Id))
	{
		TraderNameText->SetText(Merchant->DisplayName);
		DescriptionText->SetText(Merchant->Description);
		FSlateBrush PortraitBrush = TraderPortrait->GetBrush();
		PortraitBrush.SetResourceObject(Merchant->Portrait);
		PortraitBrush.SetUVRegion(FBox2D(Merchant->PortraitUVMin, Merchant->PortraitUVMax));
		TraderPortrait->SetBrush(PortraitBrush);
	}
	else
	{
		FSTPTraderDefinition Trader;
		if (GetWorld()->GetSubsystem<UTraderSubsystem>()->GetTrader(Id, Trader))
		{
			TraderNameText->SetText(Trader.Name);
			DescriptionText->SetText(Trader.Description);
		}
		if (UTradeCatalog* Catalog = Trading()->GetCatalog())
		{
			TraderPortrait->SetBrushFromTexture(Catalog->MerchantPortrait);
		}
	}
	BuildCards();
	RebuildBaskets();
	Refresh();
	SetKeyboardFocus();
}

void UTradeScreenWidget::BuildCards()
{
	TraderGrid->ClearChildren();
	PlayerGrid->ClearChildren();
	TraderCards.Empty();
	PlayerCards.Empty();
	StockItems.Empty();
	Items = Trading()->GetItems();
	if (!ItemCardClass || !BasketRowClass)
	{
		StatusText->SetText(FText::FromString(TEXT("Trading WBP templates are missing.")));
		return;
	}
	for (const FTradeItemDefinition& Item : Items)
	{
		auto AddCard = [this, &Item](UUniformGridPanel* Grid, TArray<TObjectPtr<UTradeItemWidget>>& Cards, bool bBuy)
		{
			UTradeItemWidget* Card = CreateWidget<UTradeItemWidget>(GetOwningPlayer(), ItemCardClass);
			Grid->AddChildToUniformGrid(Card, Cards.Num() / 6, Cards.Num() % 6);
			Cards.Add(Card);
			Card->Configure(Item, bBuy);
			Card->OnQuantityChanged.AddUObject(this, &UTradeScreenWidget::ChangeQuantity);
		};
		if (Trading()->GetStock(TraderId, Item.Id) > 0)
		{
			StockItems.Add(Item);
			AddCard(TraderGrid, TraderCards, true);
		}
		AddCard(PlayerGrid, PlayerCards, false);
	}
}

TArray<FTradeLine> UTradeScreenWidget::Lines(const TMap<FName, int32>& Quantities) const
{
	TArray<FTradeLine> Result;
	for (const FTradeItemDefinition& Item : Items)
	{
		if (const int32* Count = Quantities.Find(Item.Id))
		{
			Result.Add({Item.Id, *Count});
		}
	}
	return Result;
}

void UTradeScreenWidget::ChangeQuantity(FName Id, bool bBuy, int32 Delta)
{
	auto& Selection = bBuy ? Buying : Selling;
	auto& Other = bBuy ? Selling : Buying;
	const auto* Item = Items.FindByPredicate([Id](const FTradeItemDefinition& I) { return I.Id == Id; });
	if (!Item)
	{
		return;
	}
	int32 Limit = bBuy ? Trading()->GetStock(TraderId, Id) : Trading()->GetSellableCount(Id);
	if (bBuy && Item->Kind == ETradeItemKind::Blueprint)
	{
		Limit = Trading()->GetOwnedCount(Id) > 0 ? 0 : FMath::Min(Limit, 1);
	}
	if (bBuy && Item->Kind == ETradeItemKind::Drone)
	{
		Limit = FMath::Min(Limit, 20);
	}
	const int32 Count =
		Delta == MIN_int32
			? 0
			: static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Selection.FindRef(Id)) + Delta, 0, Limit));
	if (Count > 0)
	{
		Selection.Add(Id, Count);
		Other.Remove(Id);
	}
	else
	{
		Selection.Remove(Id);
	}
	LastResult = FText::GetEmpty();
	RebuildBaskets();
	Refresh();
}

void UTradeScreenWidget::RebuildBaskets()
{
	BuyingList->ClearChildren();
	SellingList->ClearChildren();
	BasketRows.Empty();
	if (!BasketRowClass)
	{
		return;
	}
	for (const auto& Item : Items)
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const bool bBuy = Side == 0;
			const int32 Count = (bBuy ? Buying : Selling).FindRef(Item.Id);
			if (!Count)
			{
				continue;
			}
			auto* Row = CreateWidget<UTradeItemWidget>(GetOwningPlayer(), BasketRowClass);
			(bBuy ? BuyingList : SellingList)->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 4));
			Row->Configure(Item, bBuy);
			Row->UpdateCounts(bBuy ? Trading()->GetStock(TraderId, Item.Id) : Trading()->GetSellableCount(Item.Id),
							  Count, true);
			Row->OnQuantityChanged.AddUObject(this, &UTradeScreenWidget::ChangeQuantity);
			BasketRows.Add(Row);
		}
	}
}

void UTradeScreenWidget::Refresh()
{
	if (TraderId.IsNone() || !Trading())
	{
		return;
	}
	for (int32 I = 0; I < TraderCards.Num(); ++I)
	{
		const auto& Item = StockItems[I];
		const bool Enabled = Item.Kind != ETradeItemKind::Blueprint || Trading()->GetOwnedCount(Item.Id) == 0;
		TraderCards[I]->UpdateCounts(Trading()->GetStock(TraderId, Item.Id), Buying.FindRef(Item.Id), Enabled);
	}
	for (int32 I = 0; I < PlayerCards.Num(); ++I)
	{
		const auto& Item = Items[I];
		PlayerCards[I]->UpdateCounts(Trading()->GetOwnedCount(Item.Id), Selling.FindRef(Item.Id),
									 Trading()->GetSellableCount(Item.Id) > Selling.FindRef(Item.Id));
	}

	if (Trading()->GetMerchant(TraderId) && !Trading()->IsTraderAvailable(TraderId))
	{
		CloseTrade();
		return;
	}
	const auto Quote = Trading()->QuoteTrade(TraderId, Lines(Buying), Lines(Selling));
	for (UTradeItemWidget* Row : BasketRows)
	{
		const FName Id = Row->GetItemId();
		const bool bBuy = Row->IsBuying();
		const int32 Available = bBuy ? Trading()->GetStock(TraderId, Id) : Trading()->GetSellableCount(Id);
		Row->UpdateCounts(Available, (bBuy ? Buying : Selling).FindRef(Id), true);
	}
	BuyingTotalText->SetText(FText::AsNumber(Quote.PurchaseTotal));
	SellingTotalText->SetText(FText::AsNumber(Quote.SaleTotal));
	if (NetCostText)
	{
		const int32 NetCost = Quote.PurchaseTotal - Quote.SaleTotal;
		NetCostText->SetText(FText::AsNumber(NetCost));
		NetCostText->SetColorAndOpacity(FSlateColor(NetCost < 0
			? FLinearColor(.04f, .60f, .70f, 1.f) : FLinearColor(1.f, .64f, .08f, 1.f)));
	}
	BalanceText->SetText(FText::AsNumber(Quote.BalanceAfter));
	for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It)
	{
		CreditsText->SetText(FText::AsNumber(It->GetCredits()));
		break;
	}
	ConfirmButton->SetIsEnabled(Quote.bCanConfirm);
	StatusText->SetText(LastResult.IsEmpty() ? Quote.Status : LastResult);
}

void UTradeScreenWidget::Confirm()
{
	FText Result;
	if (Trading()->ConfirmTrade(TraderId, Lines(Buying), Lines(Selling), Result))
	{
		Buying.Empty();
		Selling.Empty();
		BuildCards();
		RebuildBaskets();
	}
	LastResult = Result;
	Refresh();
}

void UTradeScreenWidget::Clear()
{
	Buying.Empty();
	Selling.Empty();
	LastResult = FText::GetEmpty();
	RebuildBaskets();
	Refresh();
}

void UTradeScreenWidget::CloseTrade()
{
	// Slate focus can retain the widget after removal, so release the modal
	// state immediately instead of waiting for NativeDestruct.
	RestorePlayerState();
	RemoveFromParent();
}
