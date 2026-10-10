#include "Gameplay/UI/VisitingMerchantWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Gameplay/Trading/TradeSubsystem.h"
#include "SurviveThePlanetPlayerController.h"

void UVisitingMerchantWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TradeButton->OnClicked.AddUniqueDynamic(this, &UVisitingMerchantWidget::OpenTrade);
	Refresh();
}

void UVisitingMerchantWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	RefreshElapsed += DeltaTime;
	if (RefreshElapsed >= 0.25f)
	{
		RefreshElapsed = 0.0f;
		Refresh();
	}
}

void UVisitingMerchantWidget::Refresh()
{
	const FMerchantVisitState State = GetWorld()->GetSubsystem<UTradeSubsystem>()->GetVisitState();
	MerchantPanel->SetVisibility(State.bCargoBayReady && State.Merchant
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!State.Merchant) return;
	MerchantNameText->SetText(State.Merchant->DisplayName);
	FSlateBrush Brush = MerchantPortrait->GetBrush();
	Brush.SetResourceObject(State.Merchant->Portrait);
	Brush.SetUVRegion(FBox2D(State.Merchant->PortraitUVMin, State.Merchant->PortraitUVMax));
	MerchantPortrait->SetBrush(Brush);
	const int32 Minutes = FMath::CeilToInt(State.RemainingGameMinutes);
	ArrivalText->SetText(FText::FromString(State.bPresent
		? FString::Printf(TEXT("Leaves %02d:%02d"), Minutes / 60, Minutes % 60)
		: FString::Printf(TEXT("%02d:%02d"), Minutes / 60, Minutes % 60)));
	ArrivalText->SetVisibility(ESlateVisibility::HitTestInvisible);
	const bool bCanTrade = !State.Merchant->ShipMesh || State.bShipDocked;
	TradeButton->SetIsEnabled(bCanTrade);
	if (TradeButtonLabel) TradeButtonLabel->SetText(bCanTrade
		? NSLOCTEXT("Merchant", "Trade", "TRADE")
		: NSLOCTEXT("Merchant", "Landing", "LANDING"));
	TradeButton->SetVisibility(State.bPresent ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ArrivalProgress->SetPercent(State.ArrivalProgress);
	const FString Timing = State.bPresent
		? FString::Printf(TEXT("Departs in %02dh %02dm"), Minutes / 60, Minutes % 60)
		: FString::Printf(TEXT("Arrives in %02dh %02dm"), Minutes / 60, Minutes % 60);
	MerchantPanel->SetToolTipText(FText::FromString(Timing + TEXT(" (game time)\n") + State.Merchant->Description.ToString()));
}

void UVisitingMerchantWidget::OpenTrade()
{
	const FMerchantVisitState State = GetWorld()->GetSubsystem<UTradeSubsystem>()->GetVisitState();
	if (State.bPresent && State.Merchant && GetWorld()->GetSubsystem<UTradeSubsystem>()->IsTraderAvailable(State.Merchant->Id))
	{
		if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer()))
		{
			PC->OpenTradeScreen(State.Merchant->Id);
		}
	}
}
