#include "Gameplay/UI/TraderPanelWidget.h"

#include "Blueprint/WidgetTree.h"
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
		const TCHAR* Path = nullptr;
		switch (Resource)
		{
		case EResourceType::Energy: Path = TEXT("/Game/UI/EnergyResourceIcon.EnergyResourceIcon"); break;
		case EResourceType::Iron: Path = TEXT("/Game/UI/IronResourceIcon.IronResourceIcon"); break;
		case EResourceType::ControlChip: Path = TEXT("/Game/UI/ControlChipResourceIcon.ControlChipResourceIcon"); break;
		case EResourceType::Copper: Path = TEXT("/Game/UI/CopperResourceIcon.CopperResourceIcon"); break;
		case EResourceType::Stone: Path = TEXT("/Game/UI/StoneResourceIcon.StoneResourceIcon"); break;
		case EResourceType::Concrete: Path = TEXT("/Game/UI/ConcreteResourceIcon.ConcreteResourceIcon"); break;
		case EResourceType::Water: Path = TEXT("/Game/UI/Images/WaterCollectorBuildIcon.WaterCollectorBuildIcon"); break;
		default: break;
		}
		return Path ? LoadObject<UTexture2D>(nullptr, Path) : nullptr;
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
		TradersHeading->SetText(NSLOCTEXT("SurviveThePlanet", "TradersHeading", "TRADERS"));
		STPTraderUI::StyleText(TradersHeading, 18, STPTraderUI::MainText);
	}

	TraderSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTraderSubsystem>() : nullptr;
	ContractSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UContractOfferSubsystem>() : nullptr;
	ResolveResourceManager();
	UpdateOfferStates(0.0f);
	RefreshActiveContracts(0.0f);
	RefreshTraders();
}

void UTraderPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float SimulationDeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : InDeltaTime;
	bTraderStructureDirty |= UpdateOfferStates(SimulationDeltaTime);
	ActiveContractSimulationAccumulator += SimulationDeltaTime;
	TimerRefreshAccumulator += InDeltaTime;
	if (TimerRefreshAccumulator >= 1.0f)
	{
		TimerRefreshAccumulator = 0.0f;
		RefreshActiveContracts(ActiveContractSimulationAccumulator);
		ActiveContractSimulationAccumulator = 0.0f;
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
	ResourceManager = nullptr;
	TraderSubsystem = nullptr;
	ContractSubsystem = nullptr;
	ContractOfferPopup = nullptr;
	Super::NativeDestruct();
}

void UTraderPanelWidget::BuildFallbackLayout()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TraderPanelRoot"));
	WidgetTree->RootWidget = Root;

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
			Entry.Value->SetText(FText::Format(
				NSLOCTEXT("SurviveThePlanet", "ContractOfferExpires", "Expires in: {0}"),
				FormatOfferTime(RemainingSeconds)));
		}
	}
}

void UTraderPanelWidget::RefreshActiveContracts(float DeltaTime)
{
	if (!ContractsPanel || !ContractsList || !ContractSubsystem)
	{
		return;
	}

	FName OfferId, TierId;
	if (!ContractSubsystem->IsContractSelectionLocked()
		|| !ContractSubsystem->GetSelectedContract(OfferId, TierId))
	{
		ContractsPanel->SetVisibility(ESlateVisibility::Collapsed);
		ContractsList->ClearChildren();
		TrackedContractOfferId = NAME_None;
		TrackedContractTierId = NAME_None;
		NextDeliverySecondsRemaining = 0.0f;
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

	if (TrackedContractOfferId != OfferId || TrackedContractTierId != TierId)
	{
		TrackedContractOfferId = OfferId;
		TrackedContractTierId = TierId;
		NextDeliverySecondsRemaining = Tier->DeliveryIntervalHours * 3600.0f;
	}
	else
	{
		NextDeliverySecondsRemaining = FMath::Max(0.0f, NextDeliverySecondsRemaining - FMath::Max(0.0f, DeltaTime));
	}

	ContractsPanel->SetVisibility(ESlateVisibility::Visible);
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
		STPTraderUI::StyleText(Fallback, 20, Tier->AccentColor);
		IconSize->SetContent(Fallback);
	}
	ContractRow->AddChildToHorizontalBox(IconSize)->SetPadding(FMargin(0.0f, 3.0f, 9.0f, 3.0f));

	int32 CurrentAmount = 0;
	if (const UEnum* ResourceEnum = StaticEnum<EResourceType>())
	{
		const int64 ResourceValue = ResourceEnum->GetValueByNameString(Offer.ItemId.ToString());
		if (ResourceValue != INDEX_NONE && ResourceManager)
		{
			CurrentAmount = ResourceManager->GetResourceAmount(static_cast<EResourceType>(ResourceValue));
		}
	}
	UTextBlock* Progress = WidgetTree->ConstructWidget<UTextBlock>();
	Progress->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentAmount, Tier->DeliveryAmount)));
	Progress->SetToolTipText(Offer.Title);
	STPTraderUI::StyleText(Progress, 14,
		CurrentAmount >= Tier->DeliveryAmount ? STPTraderUI::Ready : STPTraderUI::MainText);
	UHorizontalBoxSlot* ProgressSlot = ContractRow->AddChildToHorizontalBox(Progress);
	ProgressSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ProgressSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Deadline = WidgetTree->ConstructWidget<UTextBlock>();
	Deadline->SetText(FText::Format(NSLOCTEXT("SurviveThePlanet", "NextContractDelivery", "◷  {0}"),
		FormatOfferTime(NextDeliverySecondsRemaining)));
	Deadline->SetJustification(ETextJustify::Right);
	Deadline->SetToolTipText(Tier->DisplayName);
	STPTraderUI::StyleText(Deadline, 13,
		NextDeliverySecondsRemaining <= 3600.0f ? STPTraderUI::Missing : STPTraderUI::MainText);
	UHorizontalBoxSlot* DeadlineSlot = ContractRow->AddChildToHorizontalBox(Deadline);
	DeadlineSlot->SetVerticalAlignment(VAlign_Center);
	DeadlineSlot->SetHorizontalAlignment(HAlign_Right);
	ContractsList->AddChildToVerticalBox(ContractRow)->SetPadding(FMargin(0.0f, 2.0f));
}

void UTraderPanelWidget::HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount)
{
	UpdateOfferStates(0.0f);
	RefreshTraders();
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
			OfferSecondsRemaining.Add(Trader.Id, Trader.OfferDurationHours * 3600.0f);
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
	const int32 TotalMinutes = FMath::Max(0, FMath::CeilToInt(RemainingSeconds / 60.0f));
	const int32 Hours = TotalMinutes / 60;
	const int32 Minutes = TotalMinutes % 60;
	return FText::FromString(FString::Printf(TEXT("%02dh %02dm"), Hours, Minutes));
}

void UTraderPanelWidget::HandleViewOffersRequested(FName TraderId)
{
	OnViewOffersRequested.Broadcast(TraderId);
	if (!ContractOfferPopup)
	{
		ContractOfferPopup = CreateWidget<UContractOfferWidget>(GetOwningPlayer(), UContractOfferWidget::StaticClass());
		if (ContractOfferPopup) ContractOfferPopup->AddToViewport(30);
	}
	if (ContractOfferPopup) ContractOfferPopup->OpenForTrader(TraderId);
}
