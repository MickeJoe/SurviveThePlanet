#include "Gameplay/UI/ContractOfferWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Contracts/ContractOfferSubsystem.h"
#include "Gameplay/Trading/TraderSubsystem.h"

namespace STPContractUI
{
	const FLinearColor Backdrop(0.015f, 0.022f, 0.026f, 0.97f);
	const FLinearColor Card(0.025f, 0.035f, 0.039f, 0.98f);
	const FLinearColor Text(0.92f, 0.94f, 0.94f, 1.0f);
	const FLinearColor Muted(0.62f, 0.67f, 0.68f, 1.0f);
	const FLinearColor Reward(0.20f, 0.78f, 0.92f, 1.0f);
	const FLinearColor Penalty(1.0f, 0.30f, 0.22f, 1.0f);
	const FLinearColor Bonus(1.0f, 0.72f, 0.12f, 1.0f);
	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.95f);

	void Style(UTextBlock* Widget, int32 Size, const FLinearColor& Color)
	{
		if (!Widget) return;
		FSlateFontInfo Font = Widget->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = Shadow;
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(FSlateColor(Color));
		Widget->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Widget->SetShadowColorAndOpacity(Shadow);
	}
}

void UContractTierButton::InitializeTier(FName InOfferId, FName InTierId)
{
	OfferId = InOfferId;
	TierId = InTierId;
	OnClicked.AddUniqueDynamic(this, &UContractTierButton::HandleClicked);
}

void UContractTierButton::HandleClicked()
{
	OnTierClicked.Broadcast(OfferId, TierId);
}

void UContractOfferWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget) BuildLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UContractOfferWidget::BuildLayout()
{
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrushColor(STPContractUI::Backdrop);
	Root->SetPadding(FMargin(18.0f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Main);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	TraderNameText = WidgetTree->ConstructWidget<UTextBlock>();
	TraderNameText->SetJustification(ETextJustify::Center);
	STPContractUI::Style(TraderNameText, 25, STPContractUI::Text);
	UHorizontalBoxSlot* TraderSlot = Header->AddChildToHorizontalBox(TraderNameText);
	TraderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TraderSlot->SetVerticalAlignment(VAlign_Center);
	Main->AddChildToVerticalBox(Header);

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>();
	Subtitle->SetText(NSLOCTEXT("SurviveThePlanet", "ContractOfferSubtitle", "CONTRACT OFFER"));
	Subtitle->SetJustification(ETextJustify::Center);
	STPContractUI::Style(Subtitle, 18, STPContractUI::Bonus);
	Main->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	OfferTitleText = WidgetTree->ConstructWidget<UTextBlock>();
	OfferTitleText->SetJustification(ETextJustify::Center);
	STPContractUI::Style(OfferTitleText, 19, STPContractUI::Text);
	Main->AddChildToVerticalBox(OfferTitleText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 12.0f));

	OfferContent = WidgetTree->ConstructWidget<UVerticalBox>();
	Main->AddChildToVerticalBox(OfferContent);

	UButton* DoneButton = WidgetTree->ConstructWidget<UButton>();
	DoneButton->OnClicked.AddUniqueDynamic(this, &UContractOfferWidget::HandleDoneClicked);
	UTextBlock* DoneLabel = WidgetTree->ConstructWidget<UTextBlock>();
	DoneLabel->SetText(NSLOCTEXT("SurviveThePlanet", "DoneContractOffer", "DONE"));
	DoneLabel->SetJustification(ETextJustify::Center);
	STPContractUI::Style(DoneLabel, 14, STPContractUI::Text);
	DoneButton->SetContent(DoneLabel);
	UVerticalBoxSlot* DoneSlot = Main->AddChildToVerticalBox(DoneButton);
	DoneSlot->SetPadding(FMargin(390.0f, 12.0f, 390.0f, 0.0f));
}

bool UContractOfferWidget::OpenForTrader(FName TraderId)
{
	ContractSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UContractOfferSubsystem>() : nullptr;
	TraderSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTraderSubsystem>() : nullptr;
	if (!ContractSubsystem || !TraderSubsystem) return false;

	const TArray<FSTPContractOfferDefinition> Offers = ContractSubsystem->GetContractOffersForTrader(TraderId);
	FSTPTraderDefinition Trader;
	if (Offers.IsEmpty() || !TraderSubsystem->GetTrader(TraderId, Trader)) return false;

	TraderNameText->SetText(Trader.Name.ToUpper());
	PopulateOffer(Offers[0]);
	SetVisibility(ESlateVisibility::Visible);
	SetDesiredSizeInViewport(FVector2D(1000.0f, 670.0f));
	FVector2D ViewportSize(1920.0f, 1080.0f);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(ViewportSize);
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetPositionInViewport(ViewportSize * 0.5f, false);
	return true;
}

void UContractOfferWidget::PopulateOffer(const FSTPContractOfferDefinition& Offer)
{
	CurrentOfferId = Offer.Id;
	OfferTitleText->SetText(Offer.Title);
	OfferContent->ClearChildren();
	UHorizontalBox* TierRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (const FSTPContractTierDefinition& Tier : Offer.Tiers)
	{
		UHorizontalBoxSlot* TierSlot = TierRow->AddChildToHorizontalBox(BuildTierCard(Offer, Tier));
		TierSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TierSlot->SetPadding(FMargin(5.0f));
	}
	OfferContent->AddChildToVerticalBox(TierRow);
}

UBorder* UContractOfferWidget::BuildTierCard(const FSTPContractOfferDefinition& Offer,
	const FSTPContractTierDefinition& Tier)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor(STPContractUI::Card.R + Tier.AccentColor.R * 0.035f,
		STPContractUI::Card.G + Tier.AccentColor.G * 0.035f, STPContractUI::Card.B + Tier.AccentColor.B * 0.035f, 1.0f));
	Border->SetPadding(FMargin(14.0f));
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
	Border->SetContent(Card);

	UHorizontalBox* TierHeader = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
	Name->SetText(Tier.DisplayName);
	Name->SetJustification(ETextJustify::Center);
	STPContractUI::Style(Name, 18, Tier.AccentColor);
	UHorizontalBoxSlot* NameSlot = TierHeader->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	const bool bSelected = ContractSubsystem && ContractSubsystem->IsContractTierSelected(Offer.Id, Tier.Id);
	UTextBlock* Check = WidgetTree->ConstructWidget<UTextBlock>();
	Check->SetText(FText::FromString(bSelected ? TEXT("☑") : TEXT("☐")));
	Check->SetJustification(ETextJustify::Right);
	STPContractUI::Style(Check, 18, bSelected ? Tier.AccentColor : STPContractUI::Muted);
	TierHeader->AddChildToHorizontalBox(Check)->SetPadding(FMargin(6.0f, 0.0f));
	Card->AddChildToVerticalBox(TierHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	UTextBlock* Delivery = WidgetTree->ConstructWidget<UTextBlock>();
	Delivery->SetText(FText::FromString(FString::Printf(TEXT("%d  PER DELIVERY"), Tier.DeliveryAmount)));
	Delivery->SetJustification(ETextJustify::Center);
	STPContractUI::Style(Delivery, 17, STPContractUI::Text);
	Card->AddChildToVerticalBox(Delivery)->SetPadding(FMargin(0.0f, 3.0f));
	UTextBlock* Schedule = WidgetTree->ConstructWidget<UTextBlock>();
	Schedule->SetText(FText::FromString(FString::Printf(TEXT("Every %.0fh     %d DELIVERIES"),
		Tier.DeliveryIntervalHours, Tier.DeliveryCount)));
	Schedule->SetJustification(ETextJustify::Center);
	STPContractUI::Style(Schedule, 13, STPContractUI::Muted);
	Card->AddChildToVerticalBox(Schedule)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));

	AddEffectSection(Card, NSLOCTEXT("SurviveThePlanet", "ContractRewards", "REWARDS"),
		Tier.DeliveryRewards, STPContractUI::Reward);
	AddEffectSection(Card, NSLOCTEXT("SurviveThePlanet", "ContractPenalties", "PENALTIES"),
		Tier.MissedDeliveryPenalties, STPContractUI::Penalty);
	AddEffectSection(Card, NSLOCTEXT("SurviveThePlanet", "ContractBonus", "BONUS"),
		Tier.CompletionBonus, STPContractUI::Bonus);

	UContractTierButton* SelectButton = WidgetTree->ConstructWidget<UContractTierButton>();
	SelectButton->InitializeTier(Offer.Id, Tier.Id);
	SelectButton->OnTierClicked.AddUniqueDynamic(this, &UContractOfferWidget::HandleTierClicked);
	SelectButton->SetBackgroundColor(FLinearColor(Tier.AccentColor.R * 0.32f, Tier.AccentColor.G * 0.32f,
		Tier.AccentColor.B * 0.32f, 1.0f));
	UTextBlock* SelectLabel = WidgetTree->ConstructWidget<UTextBlock>();
	SelectLabel->SetText(bSelected
		? NSLOCTEXT("SurviveThePlanet", "SelectedContractTier", "SELECTED")
		: NSLOCTEXT("SurviveThePlanet", "SelectContractTier", "SELECT"));
	SelectLabel->SetJustification(ETextJustify::Center);
	STPContractUI::Style(SelectLabel, 15, Tier.AccentColor);
	SelectButton->SetContent(SelectLabel);
	Card->AddChildToVerticalBox(SelectButton)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	return Border;
}

void UContractOfferWidget::AddEffectSection(UVerticalBox* Parent, const FText& Heading,
	const TArray<FSTPContractEffectDefinition>& Effects, const FLinearColor& HeadingColor)
{
	UTextBlock* HeadingText = WidgetTree->ConstructWidget<UTextBlock>();
	HeadingText->SetText(Heading);
	HeadingText->SetJustification(ETextJustify::Center);
	STPContractUI::Style(HeadingText, 12, HeadingColor);
	Parent->AddChildToVerticalBox(HeadingText)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 3.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (const FSTPContractEffectDefinition& Effect : Effects)
	{
		if (UTexture2D* Texture = Effect.Icon.LoadSynchronous())
		{
			UImage* Icon = WidgetTree->ConstructWidget<UImage>();
			Icon->SetBrushFromTexture(Texture, true);
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
			Size->SetWidthOverride(24.0f);
			Size->SetHeightOverride(24.0f);
			Size->SetContent(Icon);
			Row->AddChildToHorizontalBox(Size)->SetPadding(FMargin(4.0f));
		}
		UTextBlock* Value = WidgetTree->ConstructWidget<UTextBlock>();
		Value->SetText(FormatEffect(Effect));
		STPContractUI::Style(Value, 13, STPContractUI::Text);
		Row->AddChildToHorizontalBox(Value)->SetPadding(FMargin(1.0f, 5.0f, 10.0f, 5.0f));
	}
	UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row);
	RowSlot->SetHorizontalAlignment(HAlign_Center);
}

FText UContractOfferWidget::FormatEffect(const FSTPContractEffectDefinition& Effect)
{
	FString Label;
	switch (Effect.Type)
	{
	case ESTPContractEffectType::Resource: Label = Effect.Id.ToString(); break;
	case ESTPContractEffectType::Blueprint: Label = TEXT("Rare Blueprint"); break;
	case ESTPContractEffectType::TraderCooldownHours: Label = TEXT("h"); break;
	default: break;
	}
	const TCHAR* Sign = Effect.Amount > 0 && Effect.Type != ESTPContractEffectType::Credits
		&& Effect.Type != ESTPContractEffectType::Resource ? TEXT("+") : TEXT("");
	return FText::FromString(FString::Printf(TEXT("%s%d%s%s"), Sign, Effect.Amount,
		Label.IsEmpty() ? TEXT("") : TEXT(" "), *Label));
}

void UContractOfferWidget::HandleCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UContractOfferWidget::HandleDoneClicked()
{
	if (ContractSubsystem)
	{
		ContractSubsystem->ConfirmSelectedContract();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UContractOfferWidget::HandleTierClicked(FName OfferId, FName TierId)
{
	if (!ContractSubsystem || !ContractSubsystem->ToggleContractSelection(OfferId, TierId)) return;
	FSTPContractOfferDefinition Offer;
	if (ContractSubsystem->GetContractOffer(CurrentOfferId, Offer)) PopulateOffer(Offer);
}
