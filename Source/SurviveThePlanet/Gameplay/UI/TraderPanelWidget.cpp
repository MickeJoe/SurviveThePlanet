#include "Gameplay/UI/TraderPanelWidget.h"
#include "Gameplay/Resources/ResourceCatalog.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Gameplay/UI/ObjectiveTrackerWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Gameplay/Trading/TraderSubsystem.h"
#include "Gameplay/Contracts/ContractOfferSubsystem.h"
#include "Gameplay/UI/ContractOfferWidget.h"

namespace STPTraderUI
{
	const FLinearColor Panel(0.035f, 0.045f, 0.050f, 0.94f);
	const FLinearColor Header(0.055f, 0.065f, 0.070f, 0.98f);
	const FLinearColor Divider(0.26f, 0.30f, 0.31f, 0.75f);
	const FLinearColor MainText(0.92f, 0.94f, 0.94f, 1.0f);
	const FLinearColor SecondaryText(0.62f, 0.67f, 0.68f, 1.0f);
	const FLinearColor Ready(0.58f, 0.88f, 0.37f, 1.0f);
	const FLinearColor Missing(1.0f, 0.29f, 0.22f, 1.0f);
	const FLinearColor Cyan(0.0f, 0.72f, 0.88f, 1.0f);
	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.95f);

	void SetTextIfChanged(UTextBlock* Widget, const FText& Text)
	{
		if (Widget && !Widget->GetText().EqualTo(Text)) Widget->SetText(Text);
	}

	void SetColorIfChanged(UTextBlock* Widget, const FLinearColor& Color)
	{
		if (Widget && Widget->GetColorAndOpacity().GetSpecifiedColor() != Color) Widget->SetColorAndOpacity(FSlateColor(Color));
	}

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
	{
		if (!Text) return;
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = Shadow;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(Shadow);
	}

	UTexture2D* GetResourceIcon(EResourceType Resource)
	{
		return UResourceCatalog::GetResourceIcon(Resource);
	}

	FLinearColor TraderAccent(int32 Index)
	{
		static const FLinearColor Colors[] = {
			FLinearColor(0.92f, 0.56f, 0.06f, 1.0f),
			FLinearColor(0.58f, 0.23f, 0.92f, 1.0f),
			FLinearColor(0.0f, 0.68f, 0.86f, 1.0f)
		};
		return Colors[Index % UE_ARRAY_COUNT(Colors)];
	}
}

void UTraderOfferButton::InitializeTrader(FName InTraderId)
{
	TraderId = InTraderId;
	OnClicked.AddUniqueDynamic(this, &UTraderOfferButton::HandleClicked);
}

void UTraderOfferButton::HandleClicked()
{
	OnTraderClicked.Broadcast(TraderId);
}

void UTraderPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
}

void UTraderPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetDesiredSizeInViewport(FVector2D(320.0f, 650.0f));
	SetPositionInViewport(FVector2D(15.0f, 155.0f), false);
	SetAlignmentInViewport(FVector2D::ZeroVector);

	if (TradersHeading)
	{
		TradersHeading->SetText(NSLOCTEXT("SurviveThePlanet", "TradersHeading", "CONTRACTS"));
		STPTraderUI::StyleText(TradersHeading, 18, STPTraderUI::MainText);
	}

	TraderSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTraderSubsystem>() : nullptr;
	ContractSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UContractOfferSubsystem>() : nullptr;
	TArray<UUserWidget*> ObjectiveWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ObjectiveWidgets, UObjectiveTrackerWidget::StaticClass(), true);
	if (!ObjectiveWidgets.IsEmpty()) ObjectiveTrackerWidget = Cast<UObjectiveTrackerWidget>(ObjectiveWidgets[0]);
	ResolveResourceManager();
	UpdateOfferStates(0.0f);
	if (ContractSubsystem) ContractSubsystem->OnContractStateChanged.AddUniqueDynamic(this, &UTraderPanelWidget::HandleContractStateChanged);
	RefreshActiveContracts();
	RefreshTraders();
	HandleContractStateChanged();
}

void UTraderPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Reward rows increase the objective tracker's height. Keep contracts below its actual content.
	const float Top = ObjectiveTrackerWidget.IsValid() ? FMath::Max(155.0f, ObjectiveTrackerWidget->GetObjectivesBottomInViewport() + 12.0f) : 155.0f;
	const float ViewportHeight = UWidgetLayoutLibrary::GetViewportSize(this).Y / UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D Position(15.0f, Top);
	const FVector2D Size(320.0f, FMath::Clamp(ViewportHeight - Top - 16.0f, 120.0f, 650.0f));
	if (!LastPanelPosition.Equals(Position, 0.1f)) { SetPositionInViewport(Position, false); LastPanelPosition = Position; }
	if (!LastPanelSize.Equals(Size, 0.1f)) { SetDesiredSizeInViewport(Size); LastPanelSize = Size; }
	const float SimulationDeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : InDeltaTime;
	bTraderStructureDirty |= UpdateOfferStates(SimulationDeltaTime);
	TimerRefreshAccumulator += InDeltaTime;
	if (TimerRefreshAccumulator >= 1.0f)
	{
		TimerRefreshAccumulator = 0.0f;
		RefreshActiveContracts();
		if (bTraderStructureDirty)
		{
			RefreshTraders();
			bTraderStructureDirty = false;
		}
		else
		{
			UpdateTraderTimerTexts();
		}
	}
}

void UTraderPanelWidget::NativeDestruct()
{
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(this, &UTraderPanelWidget::HandleResourceAmountChanged);
	}
	if (ContractSubsystem) ContractSubsystem->OnContractStateChanged.RemoveDynamic(this, &UTraderPanelWidget::HandleContractStateChanged);
	ResourceManager = nullptr;
	TraderSubsystem = nullptr;
	ContractSubsystem = nullptr;
	ContractOfferPopup = nullptr;
	Super::NativeDestruct();
}

void UTraderPanelWidget::BuildFallbackLayout()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TraderPanelRoot"));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->AddChild(Root);
	WidgetTree->RootWidget = Scroll;

	ContractsPanel = WidgetTree->ConstructWidget<UBorder>();
	ContractsPanel->SetBrushColor(STPTraderUI::Panel);
	ContractsPanel->SetPadding(FMargin(14.0f, 8.0f));
	ContractsPanel->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBox* ContractsBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ContractsPanel->SetContent(ContractsBox);
	UTextBlock* ContractsHeading = WidgetTree->ConstructWidget<UTextBlock>();
	ContractsHeading->SetText(NSLOCTEXT("SurviveThePlanet", "ContractsTrackerHeading", "CONTRACTS"));
	STPTraderUI::StyleText(ContractsHeading, 18, STPTraderUI::MainText);
	ContractsBox->AddChildToVerticalBox(ContractsHeading)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	ContractsList = WidgetTree->ConstructWidget<UVerticalBox>();
	ContractsBox->AddChildToVerticalBox(ContractsList);
	Root->AddChildToVerticalBox(ContractsPanel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>();
	HeaderBorder->SetBrushColor(STPTraderUI::Header);
	HeaderBorder->SetPadding(FMargin(14.0f, 10.0f));
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	HeaderBorder->SetContent(HeaderRow);
	TradersHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TradersHeading"));
	STPTraderUI::StyleText(TradersHeading, 18, STPTraderUI::MainText);
	UHorizontalBoxSlot* HeadingSlot = HeaderRow->AddChildToHorizontalBox(TradersHeading);
	HeadingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeadingSlot->SetVerticalAlignment(VAlign_Center);

	UButton* CollapseButton = WidgetTree->ConstructWidget<UButton>();
	CollapseButton->SetBackgroundColor(FLinearColor::Transparent);
	CollapseButton->OnClicked.AddUniqueDynamic(this, &UTraderPanelWidget::HandleCollapseClicked);
	CollapseGlyph = WidgetTree->ConstructWidget<UTextBlock>();
	CollapseGlyph->SetText(FText::FromString(TEXT("⌄")));
	STPTraderUI::StyleText(CollapseGlyph, 17, STPTraderUI::MainText);
	CollapseButton->SetContent(CollapseGlyph);
	UHorizontalBoxSlot* ButtonSlot = HeaderRow->AddChildToHorizontalBox(CollapseButton);
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
	Root->AddChildToVerticalBox(HeaderBorder);

	ContentBorder = WidgetTree->ConstructWidget<UBorder>();
	ContentBorder->SetBrushColor(STPTraderUI::Panel);
	ContentBorder->SetPadding(FMargin(14.0f, 2.0f, 14.0f, 8.0f));
	TraderList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TraderList"));
	ContentBorder->SetContent(TraderList);
	UVerticalBoxSlot* ContentSlot = Root->AddChildToVerticalBox(ContentBorder);
	ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
}

void UTraderPanelWidget::ResolveResourceManager()
{
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(this, &UTraderPanelWidget::HandleResourceAmountChanged);
	}
	ResourceManager = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			ResourceManager = *It;
			break;
		}
	}
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.AddUniqueDynamic(this, &UTraderPanelWidget::HandleResourceAmountChanged);
	}
}

void UTraderPanelWidget::RefreshTraders()
{
	if (!TraderList) return;
	OfferTimerTexts.Reset();
	TraderResourceTexts.Reset();
	TraderList->ClearChildren();
	if (!TraderSubsystem) return;

	const TArray<FSTPTraderDefinition> Traders = TraderSubsystem->GetTraders();
	for (int32 TraderIndex = 0; TraderIndex < Traders.Num(); ++TraderIndex)
	{
		const FSTPTraderDefinition& Trader = Traders[TraderIndex];
		const FLinearColor Accent = STPTraderUI::TraderAccent(TraderIndex);
		UVerticalBox* TraderBlock = WidgetTree->ConstructWidget<UVerticalBox>();
		TraderBlock->SetToolTipText(Trader.Description);

		UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		UBorder* IconFrame = WidgetTree->ConstructWidget<UBorder>();
		IconFrame->SetBrushColor(FLinearColor(Accent.R * 0.18f, Accent.G * 0.18f, Accent.B * 0.18f, 1.0f));
		IconFrame->SetPadding(FMargin(4.0f));
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
		IconSize->SetWidthOverride(34.0f);
		IconSize->SetHeightOverride(34.0f);
		IconFrame->SetContent(IconSize);
		if (UTexture2D* TraderTexture = Trader.Icon.LoadSynchronous())
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			Image->SetBrushFromTexture(TraderTexture, true);
			IconSize->SetContent(Image);
		}
		else
		{
			UTextBlock* Initial = WidgetTree->ConstructWidget<UTextBlock>();
			Initial->SetText(FText::FromString(Trader.Name.ToString().Left(1).ToUpper()));
			Initial->SetJustification(ETextJustify::Center);
			STPTraderUI::StyleText(Initial, 20, Accent);
			IconSize->SetContent(Initial);
		}
		UHorizontalBoxSlot* IconSlot = NameRow->AddChildToHorizontalBox(IconFrame);
		IconSlot->SetPadding(FMargin(0.0f, 8.0f, 12.0f, 6.0f));
		IconSlot->SetVerticalAlignment(VAlign_Center);

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
		Name->SetText(Trader.Name.ToUpper());
		STPTraderUI::StyleText(Name, 15, STPTraderUI::MainText);
		Name->SetToolTipText(Trader.Description);
		UHorizontalBoxSlot* NameSlot = NameRow->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		TraderBlock->AddChildToVerticalBox(NameRow);

		UHorizontalBox* GoodsRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (const FSTPTraderGoodDefinition& Good : Trader.Goods)
		{
			UVerticalBox* GoodBox = WidgetTree->ConstructWidget<UVerticalBox>();
			USizeBox* GoodIconSize = WidgetTree->ConstructWidget<USizeBox>();
			GoodIconSize->SetWidthOverride(46.0f);
			GoodIconSize->SetHeightOverride(46.0f);
			const UEnum* ResourceEnum = StaticEnum<EResourceType>();
			const FText ResourceName = ResourceEnum
				? ResourceEnum->GetDisplayNameTextByValue(static_cast<int64>(Good.Resource))
				: FText::FromName(UEnum::GetValueAsName(Good.Resource));
			GoodIconSize->SetToolTipText(ResourceName);
			if (UTexture2D* ResourceTexture = STPTraderUI::GetResourceIcon(Good.Resource))
			{
				UImage* ResourceImage = WidgetTree->ConstructWidget<UImage>();
				ResourceImage->SetBrushFromTexture(ResourceTexture, true);
				ResourceImage->SetToolTipText(ResourceName);
				GoodIconSize->SetContent(ResourceImage);
			}
			GoodBox->AddChildToVerticalBox(GoodIconSize)->SetHorizontalAlignment(HAlign_Center);
			const int32 CurrentAmount = ResourceManager ? ResourceManager->GetResourceAmount(Good.Resource) : 0;
			UTextBlock* Progress = WidgetTree->ConstructWidget<UTextBlock>();
			Progress->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentAmount, Good.ProductionThreshold)));
			Progress->SetJustification(ETextJustify::Center);
			STPTraderUI::StyleText(Progress, 13,
				CurrentAmount >= Good.ProductionThreshold ? STPTraderUI::Ready : STPTraderUI::Missing);
			auto& Binding = TraderResourceTexts.AddDefaulted_GetRef();
			Binding.Text = Progress; Binding.Resource = Good.Resource; Binding.RequiredAmount = Good.ProductionThreshold;
			GoodBox->AddChildToVerticalBox(Progress)->SetHorizontalAlignment(HAlign_Center);
			UHorizontalBoxSlot* GoodSlot = GoodsRow->AddChildToHorizontalBox(GoodBox);
			GoodSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			GoodSlot->SetPadding(FMargin(3.0f, 4.0f, 3.0f, 8.0f));
			GoodSlot->SetHorizontalAlignment(HAlign_Center);
		}
		TraderBlock->AddChildToVerticalBox(GoodsRow);

		const float* RemainingSeconds = OfferSecondsRemaining.Find(Trader.Id);
		if (RemainingSeconds && *RemainingSeconds > 0.0f && !ExpiredTraders.Contains(Trader.Id))
		{
			UTraderOfferButton* ViewOffersButton = WidgetTree->ConstructWidget<UTraderOfferButton>();
			ViewOffersButton->InitializeTrader(Trader.Id);
			ViewOffersButton->OnTraderClicked.AddUniqueDynamic(this, &UTraderPanelWidget::HandleViewOffersRequested);
			ViewOffersButton->SetBackgroundColor(FLinearColor(Accent.R * 0.42f, Accent.G * 0.42f, Accent.B * 0.42f, 1.0f));
			UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
			ButtonLabel->SetText(NSLOCTEXT("SurviveThePlanet", "ViewContractOffers", "VIEW OFFERS"));
			ButtonLabel->SetJustification(ETextJustify::Center);
			STPTraderUI::StyleText(ButtonLabel, 14, Accent);
			ViewOffersButton->SetContent(ButtonLabel);
			UVerticalBoxSlot* ButtonRowSlot = TraderBlock->AddChildToVerticalBox(ViewOffersButton);
			ButtonRowSlot->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 5.0f));
			ButtonRowSlot->SetHorizontalAlignment(HAlign_Fill);

			UTextBlock* Timer = WidgetTree->ConstructWidget<UTextBlock>();
			Timer->SetText(FText::Format(
				NSLOCTEXT("SurviveThePlanet", "ContractOfferExpires", "Expires in: {0}"),
				FormatOfferTime(*RemainingSeconds)));
			Timer->SetJustification(ETextJustify::Center);
			STPTraderUI::StyleText(Timer, 12, STPTraderUI::MainText);
			OfferTimerTexts.Add(Trader.Id, Timer);
			UVerticalBoxSlot* TimerSlot = TraderBlock->AddChildToVerticalBox(Timer);
			TimerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
			TimerSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		UVerticalBoxSlot* TraderSlot = TraderList->AddChildToVerticalBox(TraderBlock);
		TraderSlot->SetHorizontalAlignment(HAlign_Fill);
		if (TraderIndex + 1 < Traders.Num())
		{
			UBorder* Divider = WidgetTree->ConstructWidget<UBorder>();
			Divider->SetBrushColor(STPTraderUI::Divider);
			Divider->SetPadding(FMargin(0.0f));
			USizeBox* Line = WidgetTree->ConstructWidget<USizeBox>();
			Line->SetHeightOverride(1.0f);
			Divider->SetContent(Line);
			TraderList->AddChildToVerticalBox(Divider)->SetPadding(FMargin(0.0f, 3.0f));
		}
	}
}

void UTraderPanelWidget::UpdateTraderTimerTexts()
{
	for (const TPair<FName, TObjectPtr<UTextBlock>>& Entry : OfferTimerTexts)
	{
		if (Entry.Value)
		{
			const float RemainingSeconds = OfferSecondsRemaining.FindRef(Entry.Key);
			STPTraderUI::SetTextIfChanged(Entry.Value, FText::Format(
				NSLOCTEXT("SurviveThePlanet", "ContractOfferExpires", "Expires in: {0}"),
				FormatOfferTime(RemainingSeconds)));
		}
	}
}

void UTraderPanelWidget::BuildActiveContractWidgets(const FSTPContractOfferDefinition& Offer, const FSTPContractTierDefinition& Tier)
{
	ContractsList->ClearChildren();
	UHorizontalBox* ContractRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	ContractRow->SetToolTipText(Offer.Title);

	USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
	IconSize->SetWidthOverride(34.0f);
	IconSize->SetHeightOverride(34.0f);
	if (UTexture2D* Texture = Offer.Icon.LoadSynchronous())
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>();
		Icon->SetBrushFromTexture(Texture, true);
		IconSize->SetContent(Icon);
	}
	else
	{
		UTextBlock* Fallback = WidgetTree->ConstructWidget<UTextBlock>();
		Fallback->SetText(FText::FromString(TEXT("◆")));
		Fallback->SetJustification(ETextJustify::Center);
		STPTraderUI::StyleText(Fallback, 20, Tier.AccentColor);
		IconSize->SetContent(Fallback);
	}
	ContractRow->AddChildToHorizontalBox(IconSize)->SetPadding(FMargin(0.0f, 3.0f, 9.0f, 3.0f));

	ActiveProgressText = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* Progress = ActiveProgressText;
	STPTraderUI::StyleText(Progress, 14, STPTraderUI::MainText);
	UHorizontalBoxSlot* ProgressSlot = ContractRow->AddChildToHorizontalBox(Progress);
	ProgressSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ProgressSlot->SetVerticalAlignment(VAlign_Center);

	ActiveDeadlineText = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* Deadline = ActiveDeadlineText;
	Deadline->SetJustification(ETextJustify::Right);
	STPTraderUI::StyleText(Deadline, 13, STPTraderUI::MainText);
	UHorizontalBoxSlot* DeadlineSlot = ContractRow->AddChildToHorizontalBox(Deadline);
	DeadlineSlot->SetVerticalAlignment(VAlign_Center);
	DeadlineSlot->SetHorizontalAlignment(HAlign_Right);
	ContractsList->AddChildToVerticalBox(ContractRow)->SetPadding(FMargin(0.0f, 2.0f));
	ActiveDeliveryStatusText = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* DeliveryStatus = ActiveDeliveryStatusText;
	STPTraderUI::StyleText(DeliveryStatus, 12, STPTraderUI::MainText);
	ContractsList->AddChildToVerticalBox(DeliveryStatus);
	ActiveRewardText = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* ProgressReward = ActiveRewardText;
	STPTraderUI::StyleText(ProgressReward, 11, STPTraderUI::Cyan);
	ProgressReward->SetAutoWrapText(true);
	ContractsList->AddChildToVerticalBox(ProgressReward)->SetPadding(FMargin(0, 3));
	{
		ActiveDeliverButton = WidgetTree->ConstructWidget<UButton>();
		UButton* Deliver = ActiveDeliverButton;
		Deliver->OnClicked.AddUniqueDynamic(this, &UTraderPanelWidget::HandleDeliverContractClicked);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(NSLOCTEXT("SurviveThePlanet", "DeliverAgreementShipment", "DELIVER SHIPMENT"));
		Label->SetJustification(ETextJustify::Center);
		STPTraderUI::StyleText(Label, 12, STPTraderUI::Cyan);
		Deliver->SetContent(Label);
		ContractsList->AddChildToVerticalBox(Deliver)->SetPadding(FMargin(0, 5));
	}
}

void UTraderPanelWidget::RefreshActiveContracts()
{
	if (!ContractsPanel || !ContractsList || !ContractSubsystem)
	{
		return;
	}

	FName OfferId, TierId;
	const FSTPContractRuntimeState ContractState = ContractSubsystem->GetSelectedContractState();
	if (ContractState.State == ESTPContractState::None || !ContractSubsystem->GetSelectedContract(OfferId, TierId))
	{
		ContractsPanel->SetVisibility(ESlateVisibility::Collapsed);
		if (!ActiveOfferId.IsNone()) ContractsList->ClearChildren();
		ActiveOfferId = NAME_None; ActiveTierId = NAME_None;
		ActiveProgressText = nullptr; ActiveDeadlineText = nullptr; ActiveDeliveryStatusText = nullptr;
		ActiveRewardText = nullptr; ActiveDeliverButton = nullptr;
		return;
	}

	FSTPContractOfferDefinition Offer;
	if (!ContractSubsystem->GetContractOffer(OfferId, Offer))
	{
		ContractsPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FSTPContractTierDefinition* Tier = Offer.Tiers.FindByPredicate(
		[TierId](const FSTPContractTierDefinition& Candidate) { return Candidate.Id == TierId; });
	if (!Tier)
	{
		ContractsPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}


	ContractsPanel->SetVisibility(ESlateVisibility::Visible);
	if (ActiveOfferId != OfferId || ActiveTierId != TierId || !ActiveProgressText)
	{
		BuildActiveContractWidgets(Offer, *Tier);
		ActiveOfferId = OfferId; ActiveTierId = TierId;
	}
	int32 CurrentAmount = 0;
	if (const UEnum* ResourceEnum = StaticEnum<EResourceType>())
	{
		const int64 ResourceValue = ResourceEnum->GetValueByNameString(Offer.ItemId.ToString());
		if (ResourceValue != INDEX_NONE && ResourceManager)
		{
			CurrentAmount = ResourceManager->GetResourceAmount(static_cast<EResourceType>(ResourceValue));
		}
	}
	UTextBlock* Progress = ActiveProgressText;
	UTextBlock* Deadline = ActiveDeadlineText;
	UTextBlock* DeliveryStatus = ActiveDeliveryStatusText;
	UTextBlock* ProgressReward = ActiveRewardText;
	const bool bActive = ContractState.State == ESTPContractState::Active;
	STPTraderUI::SetTextIfChanged(Progress, bActive
		? FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentAmount, Tier->DeliveryAmount))
		: FText::FromString(ContractState.State == ESTPContractState::Completed ? TEXT("FULFILLED") : TEXT("ENDED")));
	Progress->SetToolTipText(Offer.Title);
	STPTraderUI::SetColorIfChanged(Progress, CurrentAmount >= Tier->DeliveryAmount ? STPTraderUI::Ready : STPTraderUI::MainText);
	STPTraderUI::SetTextIfChanged(Deadline, bActive
		? FText::Format(NSLOCTEXT("SurviveThePlanet", "NextContractDelivery", "\u25F7  {0}"), FormatOfferTime(ContractState.SecondsUntilDelivery))
		: FText::GetEmpty());
	Deadline->SetToolTipText(Tier->DisplayName);
	STPTraderUI::SetColorIfChanged(Deadline, ContractState.SecondsUntilDelivery <= 60.0f ? STPTraderUI::Missing : STPTraderUI::MainText);
	STPTraderUI::SetTextIfChanged(DeliveryStatus, FText::FromString(FString::Printf(TEXT("%d / %d DELIVERIES%s"), ContractState.DeliveriesMade,
		Tier->DeliveryCount, ContractState.State == ESTPContractState::Completed ? TEXT(" - COMPLETE") :
		ContractState.State == ESTPContractState::Failed ? TEXT(" - FAILED") : TEXT(""))));
	FText RewardText;
	if (ContractState.State == ESTPContractState::Failed)
		RewardText = NSLOCTEXT("SurviveThePlanet", "NoAgreementCompletionReward", "NO COMPLETION REWARD");
	else
		RewardText = FText::Format(ContractState.State == ESTPContractState::Completed
			? NSLOCTEXT("SurviveThePlanet", "ContractProgressReceived", "+{0} MISSION PROGRESSION RECEIVED")
			: NSLOCTEXT("SurviveThePlanet", "ContractProgressOnCompletion", "+{0} MISSION PROGRESSION ON COMPLETION"),
			FText::AsNumber(Tier->GetMissionProgressReward()));
	STPTraderUI::SetTextIfChanged(ProgressReward, RewardText);
	ActiveDeliverButton->SetVisibility(ContractState.State == ESTPContractState::Active ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ActiveDeliverButton->SetIsEnabled(ContractSubsystem->CanDeliverSelectedContract());

}

void UTraderPanelWidget::UpdateTraderResourceTexts(EResourceType Resource)
{
	const int32 Amount = ResourceManager ? ResourceManager->GetResourceAmount(Resource) : 0;
	for (const auto& Binding : TraderResourceTexts)
	{
		if (Binding.Resource != Resource || !Binding.Text) continue;
		STPTraderUI::SetTextIfChanged(Binding.Text, FText::FromString(FString::Printf(TEXT("%d / %d"), Amount, Binding.RequiredAmount)));
		STPTraderUI::SetColorIfChanged(Binding.Text, Amount >= Binding.RequiredAmount ? STPTraderUI::Ready : STPTraderUI::Missing);
	}
}

void UTraderPanelWidget::HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount)
{
	bTraderStructureDirty |= UpdateOfferStates(0.0f);
	UpdateTraderResourceTexts(ResourceType);
	RefreshActiveContracts();
}

void UTraderPanelWidget::HandleCollapseClicked()
{
	bCollapsed = !bCollapsed;
	if (ContentBorder) ContentBorder->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (CollapseGlyph) CollapseGlyph->SetText(FText::FromString(bCollapsed ? TEXT("⌃") : TEXT("⌄")));
}

bool UTraderPanelWidget::IsTraderEligible(const FSTPTraderDefinition& Trader) const
{
	if (!ResourceManager || !ContractSubsystem || ContractSubsystem->IsContractSelectionLocked()
		|| ContractSubsystem->GetTraderCooldownSeconds(Trader.Id) > 0.0f
		|| ContractSubsystem->GetContractOffersForTrader(Trader.Id).IsEmpty()) return false;
	for (const FSTPTraderGoodDefinition& Good : Trader.Goods)
	{
		if (ResourceManager->GetResourceAmount(Good.Resource) >= Good.ProductionThreshold)
		{
			return true;
		}
	}
	return false;
}

bool UTraderPanelWidget::UpdateOfferStates(float DeltaTime)
{
	if (!TraderSubsystem) return false;
	bool bStructureChanged = false;
	for (const FSTPTraderDefinition& Trader : TraderSubsystem->GetTraders())
	{
		const bool bEligible = IsTraderEligible(Trader);
		const bool bWasEligible = PreviouslyEligibleTraders.Contains(Trader.Id);
		if (!bEligible)
		{
			bStructureChanged |= bWasEligible || OfferSecondsRemaining.Contains(Trader.Id);
			PreviouslyEligibleTraders.Remove(Trader.Id);
			ExpiredTraders.Remove(Trader.Id);
			OfferSecondsRemaining.Remove(Trader.Id);
			continue;
		}

		if (!bWasEligible && !ExpiredTraders.Contains(Trader.Id))
		{
			OfferSecondsRemaining.Add(Trader.Id, Trader.OfferDurationHours * 60.0f);
			bStructureChanged = true;
		}
		PreviouslyEligibleTraders.Add(Trader.Id);

		if (float* Remaining = OfferSecondsRemaining.Find(Trader.Id))
		{
			*Remaining = FMath::Max(0.0f, *Remaining - FMath::Max(0.0f, DeltaTime));
			if (*Remaining <= 0.0f)
			{
				OfferSecondsRemaining.Remove(Trader.Id);
				ExpiredTraders.Add(Trader.Id);
				bStructureChanged = true;
			}
		}
	}
	return bStructureChanged;
}

FText UTraderPanelWidget::FormatOfferTime(float RemainingSeconds)
{
	const int32 TotalMinutes = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	const int32 Hours = TotalMinutes / 60;
	const int32 Minutes = TotalMinutes % 60;
	return FText::FromString(FString::Printf(TEXT("%02dh %02dm"), Hours, Minutes));
}

void UTraderPanelWidget::HandleViewOffersRequested(FName TraderId)
{
	if (!ContractSubsystem || !ContractSubsystem->AreContractsAvailable()) return;
	OnViewOffersRequested.Broadcast(TraderId);
	if (!ContractOfferPopup)
	{
		UClass* PopupClass = LoadClass<UContractOfferWidget>(nullptr, TEXT("/Game/UI/Contracts/WBP_ContractOffer.WBP_ContractOffer_C"));
		if (PopupClass) ContractOfferPopup = CreateWidget<UContractOfferWidget>(GetOwningPlayer(), PopupClass);
		if (ContractOfferPopup) ContractOfferPopup->AddToViewport(30);
	}
	if (ContractOfferPopup) ContractOfferPopup->OpenForTrader(TraderId);
}

void UTraderPanelWidget::HandleContractStateChanged()
{
	const bool bAvailable = ContractSubsystem && ContractSubsystem->AreContractsAvailable();
	SetVisibility(bAvailable ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	// Refresh on the next UI tick; a resource callback may still be inside settlement.
	bTraderStructureDirty = true;
	TimerRefreshAccumulator = 1.0f;
}

void UTraderPanelWidget::HandleDeliverContractClicked()
{
	if (ContractSubsystem) ContractSubsystem->DeliverSelectedContract();
}
