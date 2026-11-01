#include "Gameplay/UI/MissionChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace STPMissionChoices
{
	const FLinearColor DefaultCard(0.035f, 0.075f, 0.095f, 1.0f);
	const FLinearColor Gold(0.82f, 0.67f, 0.32f, 1.0f);
	const FLinearColor White(0.94f, 0.96f, 0.97f, 1.0f);
	const FLinearColor Cyan(0.10f, 0.72f, 0.94f, 1.0f);

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(Color);
		Text->SetShadowOffset(FVector2D(1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f));
	}

	FText FormatReward(const FSTPObjectiveRewardDefinition& Reward, const UObjectiveSubsystem* ObjectiveSubsystem)
	{
		switch (Reward.Type)
		{
		case ESTPObjectiveRewardType::GiveResource:
		{
			const UEnum* ResourceEnum = StaticEnum<EResourceType>();
			const FText Name = ResourceEnum
				? ResourceEnum->GetDisplayNameTextByValue(static_cast<int64>(Reward.ResourceType))
				: FText::FromString(TEXT("Resource"));
			return FText::Format(NSLOCTEXT("SurviveThePlanet", "ObjectiveRewardResource", "+{0} {1}"),
				FText::AsNumber(Reward.Amount), Name);
		}
		case ESTPObjectiveRewardType::GiveMissionProgress:
			return FText::Format(NSLOCTEXT("SurviveThePlanet", "ObjectiveRewardProgression", "+{0} MISSION PROGRESSION"),
				FText::AsNumber(Reward.Amount));
		case ESTPObjectiveRewardType::GiveMissionConfidence:
			return FText::Format(NSLOCTEXT("SurviveThePlanet", "ObjectiveRewardConfidence", "+{0} MISSION CONFIDENCE"),
				FText::AsNumber(Reward.Amount));
		case ESTPObjectiveRewardType::UnlockObjective:
		{
			FSTPObjectiveDefinition Unlocked;
			const FText Name = ObjectiveSubsystem && ObjectiveSubsystem->GetObjectiveDefinition(Reward.ObjectiveId, Unlocked)
				? Unlocked.Title : FText::FromName(Reward.ObjectiveId);
			return FText::Format(NSLOCTEXT("SurviveThePlanet", "ObjectiveRewardUnlock", "UNLOCKS: {0}"), Name);
		}
		default:
			return FText::GetEmpty();
		}
	}

	void AddRewards(UWidgetTree* Tree, UVerticalBox* Card, const FSTPObjectiveDefinition& Definition,
		const UObjectiveSubsystem* ObjectiveSubsystem, int32 FontSize = 11)
	{
		if (Definition.Rewards.IsEmpty()) return;
		UTextBlock* Heading = Tree->ConstructWidget<UTextBlock>();
		Heading->SetText(NSLOCTEXT("SurviveThePlanet", "ObjectiveRewardsTitle", "REWARDS"));
		StyleText(Heading, FontSize, Gold);
		Card->AddChildToVerticalBox(Heading)->SetPadding(FMargin(10, 2, 10, 1));
		for (const FSTPObjectiveRewardDefinition& Reward : Definition.Rewards)
		{
			UTextBlock* Line = Tree->ConstructWidget<UTextBlock>();
			Line->SetText(FormatReward(Reward, ObjectiveSubsystem));
			Line->SetAutoWrapText(true);
			StyleText(Line, FontSize, White);
			Card->AddChildToVerticalBox(Line)->SetPadding(FMargin(10, 0, 10, 1));
		}
	}

	USizeBox* SizeCard(UWidgetTree* Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(Width);
		Size->SetMinDesiredHeight(Height);
		Size->SetContent(Content);
		return Size;
	}
}

void UMissionChoiceButton::InitializeChoice(UMissionChoiceWidget* InOwner, int32 InSlotIndex, FName InObjectiveId)
{
	OwnerWidget = InOwner;
	SlotIndex = InSlotIndex;
	ObjectiveId = InObjectiveId;
	OnClicked.AddUniqueDynamic(this, &UMissionChoiceButton::HandleClicked);
}

void UMissionChoiceButton::HandleClicked()
{
	if (OwnerWidget) OwnerWidget->HandleCandidateClicked(SlotIndex, ObjectiveId);
}

void UMissionChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	// A full-screen hit target catches clicks outside the panel and above the HUD.
	UButton* Blocker = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MissionModalBlocker"));
	Blocker->SetBackgroundColor(FLinearColor::Transparent);
	UCanvasPanelSlot* BlockerSlot = ModalOverlay->AddChildToCanvas(Blocker);
	BlockerSlot->SetAnchors(FAnchors(0, 0, 1, 1));
	BlockerSlot->SetOffsets(FMargin(0));
	BlockerSlot->SetZOrder(-100);
	ObjectiveSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UObjectiveSubsystem>() : nullptr;
	OpenMissionChoicesButton->OnClicked.AddUniqueDynamic(this, &UMissionChoiceWidget::OpenPopup);
	CloseMissionChoicesButton->OnClicked.AddUniqueDynamic(this, &UMissionChoiceWidget::ClosePopup);
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnMissionChoiceConfirmed.AddUniqueDynamic(this, &UMissionChoiceWidget::HandleGroupConfirmed);
	}
	SetPopupVisible(false);
}

void UMissionChoiceWidget::NativeDestruct()
{
	SetPopupVisible(false);
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnMissionChoiceConfirmed.RemoveDynamic(this, &UMissionChoiceWidget::HandleGroupConfirmed);
	}
	Super::NativeDestruct();
}

void UMissionChoiceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
 Super::NativeTick(MyGeometry, InDeltaTime);
 if (!ObjectiveSubsystem || !ModalOverlay->IsVisible()) return;
 TArray<UWidget*> Widgets;
 WidgetTree->GetAllWidgets(Widgets);
 for (UWidget* Widget : Widgets)
  if (auto* Button = Cast<UMissionChoiceButton>(Widget); Button && Button->GetSlotIndex() == INDEX_NONE)
   Button->SetIsEnabled(ObjectiveSubsystem->CanDeliverObjective(Button->GetObjectiveId()));
}

void UMissionChoiceWidget::OpenPopup()
{
	BuildCurrentGroup();
	SetPopupVisible(true);
}

bool UMissionChoiceWidget::ClosePopupIfOpen()
{
	if (!ModalOverlay || !ModalOverlay->IsVisible()) return false;
	ClosePopup();
	return true;
}

void UMissionChoiceWidget::ClosePopup() { SetPopupVisible(false); }

void UMissionChoiceWidget::SetPopupVisible(bool bVisible)
{
	ModalOverlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bPopupOpen == bVisible) return;
	bPopupOpen = bVisible;
	auto* PC = GetOwningPlayer();
	if (!PC) return;
	if (bVisible)
	{
		PreviousTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
		bPreviouslyPaused = UGameplayStatics::IsGamePaused(this);
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UResourceDisplayWidget::StaticClass(), false);
		for (auto* Widget : Widgets) if (Widget->GetOwningPlayer() == PC && Widget->IsInViewport())
		{
			PausedHUD = Cast<UResourceDisplayWidget>(Widget);
			PausedHUD->SetMenuPaused(true);
			break;
		}
		PC->SetPause(true);
		PC->FlushPressedKeys();
		if (auto* Viewport = UGameViewportSubsystem::Get())
		{
			auto ViewportSlot = Viewport->GetWidgetSlot(this);
			ViewportSlot.ZOrder = 1500;
			Viewport->SetWidgetSlot(this, ViewportSlot);
		}
		PC->SetInputMode(FInputModeUIOnly().SetWidgetToFocus(TakeWidget()));
		PC->bShowMouseCursor = true;
		CloseMissionChoicesButton->SetKeyboardFocus();
	}
	else
	{
		if (PausedHUD) PausedHUD->SetMenuPaused(false);
		PausedHUD = nullptr;
		UGameplayStatics::SetGlobalTimeDilation(this, PreviousTimeDilation);
		PC->SetPause(bPreviouslyPaused);
		PC->FlushPressedKeys();
		if (auto* Viewport = UGameViewportSubsystem::Get())
		{
			auto ViewportSlot = Viewport->GetWidgetSlot(this);
			ViewportSlot.ZOrder = 20;
			Viewport->SetWidgetSlot(this, ViewportSlot);
		}
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
	}
}

FReply UMissionChoiceWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (bPopupOpen && Event.GetKey() == EKeys::Escape)
	{
		ClosePopup();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UMissionChoiceWidget::BuildCurrentGroup()
{
	ChoiceColumns->ClearChildren();
	if (!ObjectiveSubsystem) return;
	const auto Groups = ObjectiveSubsystem->GetMissionChoiceGroups();
	CurrentGroup = Groups.IsEmpty() ? FSTPMissionChoiceGroupDefinition() : Groups[0];
	PickerSlotIndex = INDEX_NONE;
	HeaderText->SetVisibility(ESlateVisibility::Collapsed);
	DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
	ChoiceStatusText->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmChoicesButton->SetVisibility(ESlateVisibility::Collapsed);

	FSTPMissionChoiceGroupState State;
	ObjectiveSubsystem->GetMissionChoiceGroupState(CurrentGroup.Id, State);
	State.SelectedObjectiveIds.SetNum(CurrentGroup.Slots.Num());

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(6));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->AddChild(Grid);
	UHorizontalBoxSlot* ContainerSlot = ChoiceColumns->AddChildToHorizontalBox(Scroll);
	ContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ContainerSlot->SetHorizontalAlignment(HAlign_Center);
	ContainerSlot->SetVerticalAlignment(VAlign_Fill);

	int32 CardIndex = 0;
	for (const FSTPObjectiveRuntimeState& Runtime : ObjectiveSubsystem->GetActiveObjectives())
	{

		FSTPObjectiveDefinition Definition;
		if (!ObjectiveSubsystem->GetObjectiveDefinition(Runtime.ObjectiveId, Definition)) continue;
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetBrushColor(STPMissionChoices::DefaultCard);
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(Definition.Title);
		Title->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Title, 16, STPMissionChoices::Cyan);
		Card->AddChildToVerticalBox(Title)->SetPadding(FMargin(10, 8, 10, 3));
		UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>();
		Description->SetText(Definition.Description);
		Description->SetWrapTextAt(236.0f);
		Description->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Description, 13, STPMissionChoices::White);
		Card->AddChildToVerticalBox(Description)->SetPadding(FMargin(10, 0, 10, 4));
		if (!Definition.Conditions.IsEmpty())
		{
			const int32 Current = Runtime.ConditionProgress.IsValidIndex(0) ? Runtime.ConditionProgress[0] : 0;
			UTextBlock* Progress = WidgetTree->ConstructWidget<UTextBlock>();
			Progress->SetText(FText::Format(NSLOCTEXT("SurviveThePlanet", "ObjectiveOverviewProgress", "PROGRESS {0} / {1}"),
				FText::AsNumber(Current), FText::AsNumber(Definition.Conditions[0].RequiredAmount)));
			STPMissionChoices::StyleText(Progress, 11, STPMissionChoices::Gold);
			Card->AddChildToVerticalBox(Progress)->SetPadding(FMargin(10, 0, 10, 3));
		}
		STPMissionChoices::AddRewards(WidgetTree, Card, Definition, ObjectiveSubsystem, 12);
		if (!Definition.Conditions.IsEmpty() && Definition.Conditions[0].Type == ESTPObjectiveConditionType::DeliverResource)
		{
			UMissionChoiceButton* Deliver = WidgetTree->ConstructWidget<UMissionChoiceButton>();
			Deliver->InitializeChoice(this, INDEX_NONE, Definition.Id);
			Deliver->SetIsEnabled(ObjectiveSubsystem->CanDeliverObjective(Definition.Id));
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(NSLOCTEXT("STP", "DeliverMotherBaseSupply", "DELIVER TO MOTHER BASE"));
			Label->SetAutoWrapText(true);
			Label->SetJustification(ETextJustify::Center);
			STPMissionChoices::StyleText(Label, 11, STPMissionChoices::Cyan);
			Deliver->SetContent(Label);
			Card->AddChildToVerticalBox(Deliver)->SetPadding(FMargin(10, 5));
		}
		Border->SetContent(Card);
		Grid->AddChildToUniformGrid(STPMissionChoices::SizeCard(WidgetTree, Border, 260.0f, 400.0f), CardIndex / 3, CardIndex % 3);
		++CardIndex;
	}

	for (int32 SlotIndex = 0; SlotIndex < CurrentGroup.Slots.Num() && CardIndex < 8; ++SlotIndex)
	{
		if (State.SelectedObjectiveIds.IsValidIndex(SlotIndex) && !State.SelectedObjectiveIds[SlotIndex].IsNone()) continue;
		UMissionChoiceButton* Button = WidgetTree->ConstructWidget<UMissionChoiceButton>();
		Button->InitializeChoice(this, SlotIndex, NAME_None);
		Button->SetBackgroundColor(STPMissionChoices::DefaultCard);
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* SlotTitle = WidgetTree->ConstructWidget<UTextBlock>();
		SlotTitle->SetText(CurrentGroup.Slots[SlotIndex].Title);
		SlotTitle->SetAutoWrapText(true);
		SlotTitle->SetJustification(ETextJustify::Center);
		STPMissionChoices::StyleText(SlotTitle, 13, STPMissionChoices::Gold);
		Card->AddChildToVerticalBox(SlotTitle)->SetPadding(FMargin(10, 30, 10, 20));
		UTextBlock* Select = WidgetTree->ConstructWidget<UTextBlock>();
		Select->SetText(NSLOCTEXT("SurviveThePlanet", "OpenObjectivePickerButton", "VÃƒÆ’Ã¢â‚¬Å¾LJ OBJECTIVE"));
		Select->SetJustification(ETextJustify::Center);
		STPMissionChoices::StyleText(Select, 15, STPMissionChoices::Cyan);
		Card->AddChildToVerticalBox(Select)->SetPadding(FMargin(10, 10, 10, 25));
		Button->AddChild(Card);
		Grid->AddChildToUniformGrid(STPMissionChoices::SizeCard(WidgetTree, Button, 205.0f, 250.0f), CardIndex / 4, CardIndex % 4);
		++CardIndex;
	}
}

void UMissionChoiceWidget::HandleCandidateClicked(int32 SlotIndex, FName ObjectiveId)
{
	if (SlotIndex == INDEX_NONE && !ObjectiveId.IsNone())
	{
		if (ObjectiveSubsystem) ObjectiveSubsystem->DeliverObjective(ObjectiveId);
		BuildCurrentGroup();
		return;
	}
	if (ObjectiveId.IsNone())
	{
		BuildSlotPicker(SlotIndex);
		return;
	}
	if (!ObjectiveSubsystem || !ObjectiveSubsystem->ConfirmMissionChoiceSlot(CurrentGroup.Id, SlotIndex, ObjectiveId))
	{
		ChoiceStatusText->SetVisibility(ESlateVisibility::Visible);
		ChoiceStatusText->SetText(NSLOCTEXT("SurviveThePlanet", "MissionChoiceSlotFailed", "Could not activate this objective."));
	}
}

void UMissionChoiceWidget::BuildSlotPicker(int32 SlotIndex)
{
	if (!ObjectiveSubsystem || !CurrentGroup.Slots.IsValidIndex(SlotIndex)) return;
	PickerSlotIndex = SlotIndex;
	ChoiceColumns->ClearChildren();
	HeaderText->SetVisibility(ESlateVisibility::Visible);
	HeaderText->SetText(CurrentGroup.Slots[SlotIndex].Title);
	DescriptionText->SetVisibility(ESlateVisibility::Visible);
	DescriptionText->SetText(NSLOCTEXT("SurviveThePlanet", "ObjectivePickerDescription", "Choose one objective. Your choice is permanent."));
	ChoiceStatusText->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmChoicesButton->SetVisibility(ESlateVisibility::Collapsed);

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Grid->SetSlotPadding(FMargin(8));
	UHorizontalBoxSlot* ContainerSlot = ChoiceColumns->AddChildToHorizontalBox(Grid);
	ContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ContainerSlot->SetHorizontalAlignment(HAlign_Center);
	ContainerSlot->SetVerticalAlignment(VAlign_Center);

	int32 Index = 0;
	for (const FName ObjectiveId : CurrentGroup.Slots[SlotIndex].ObjectiveIds)
	{
		FSTPObjectiveDefinition Definition;
		if (!ObjectiveSubsystem->GetObjectiveDefinition(ObjectiveId, Definition)) continue;
		UMissionChoiceButton* Button = WidgetTree->ConstructWidget<UMissionChoiceButton>();
		Button->InitializeChoice(this, SlotIndex, ObjectiveId);
		Button->SetBackgroundColor(STPMissionChoices::DefaultCard);
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(Definition.Title);
		Title->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Title, 16, STPMissionChoices::Cyan);
		Card->AddChildToVerticalBox(Title)->SetPadding(FMargin(12, 10, 12, 5));
		UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>();
		Description->SetText(Definition.Description);
		Description->SetWrapTextAt(236.0f);
		Description->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Description, 12, STPMissionChoices::White);
		Card->AddChildToVerticalBox(Description)->SetPadding(FMargin(12, 0, 12, 6));
		STPMissionChoices::AddRewards(WidgetTree, Card, Definition, ObjectiveSubsystem, 12);
		UTextBlock* Select = WidgetTree->ConstructWidget<UTextBlock>();
		Select->SetText(NSLOCTEXT("SurviveThePlanet", "ConfirmSingleObjective", "VÃƒÆ’Ã¢â‚¬Å¾LJ"));
		Select->SetJustification(ETextJustify::Center);
		STPMissionChoices::StyleText(Select, 14, STPMissionChoices::Gold);
		Card->AddChildToVerticalBox(Select)->SetPadding(FMargin(12, 6, 12, 10));
		Button->AddChild(Card);
		Grid->AddChildToUniformGrid(STPMissionChoices::SizeCard(WidgetTree, Button, 260.0f, 240.0f), Index / 3, Index % 3);
		++Index;
	}
}

void UMissionChoiceWidget::HandleGroupConfirmed(FName ChoiceGroupId)
{
	if (ChoiceGroupId == CurrentGroup.Id) BuildCurrentGroup();
}
