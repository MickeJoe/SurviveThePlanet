#include "Gameplay/UI/MissionChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
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
		Size->SetHeightOverride(Height);
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
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnMissionChoiceConfirmed.RemoveDynamic(this, &UMissionChoiceWidget::HandleGroupConfirmed);
	}
	Super::NativeDestruct();
}

void UMissionChoiceWidget::OpenPopup()
{
	BuildCurrentGroup();
	SetPopupVisible(true);
}

void UMissionChoiceWidget::ClosePopup() { SetPopupVisible(false); }

void UMissionChoiceWidget::SetPopupVisible(bool bVisible)
{
	ModalOverlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMissionChoiceWidget::BuildCurrentGroup()
{
	ChoiceColumns->ClearChildren();
	if (!ObjectiveSubsystem || ObjectiveSubsystem->GetMissionChoiceGroups().IsEmpty()) return;
	CurrentGroup = ObjectiveSubsystem->GetMissionChoiceGroups()[0];
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
	UHorizontalBoxSlot* ContainerSlot = ChoiceColumns->AddChildToHorizontalBox(Grid);
	ContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ContainerSlot->SetHorizontalAlignment(HAlign_Center);
	ContainerSlot->SetVerticalAlignment(VAlign_Center);

	int32 CardIndex = 0;
	for (const FSTPObjectiveRuntimeState& Runtime : ObjectiveSubsystem->GetActiveObjectives())
	{
		if (CardIndex >= 8) break;
		FSTPObjectiveDefinition Definition;
		if (!ObjectiveSubsystem->GetObjectiveDefinition(Runtime.ObjectiveId, Definition)) continue;
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetBrushColor(STPMissionChoices::DefaultCard);
		UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>();
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
		Title->SetText(Definition.Title);
		Title->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Title, 15, STPMissionChoices::Cyan);
		Card->AddChildToVerticalBox(Title)->SetPadding(FMargin(10, 8, 10, 3));
		UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>();
		Description->SetText(Definition.Description);
		Description->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Description, 11, STPMissionChoices::White);
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
		STPMissionChoices::AddRewards(WidgetTree, Card, Definition, ObjectiveSubsystem);
		Border->SetContent(Card);
		Grid->AddChildToUniformGrid(STPMissionChoices::SizeCard(WidgetTree, Border, 205.0f, 250.0f), CardIndex / 4, CardIndex % 4);
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
		Select->SetText(NSLOCTEXT("SurviveThePlanet", "OpenObjectivePickerButton", "VÄLJ OBJECTIVE"));
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
		Description->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Description, 12, STPMissionChoices::White);
		Card->AddChildToVerticalBox(Description)->SetPadding(FMargin(12, 0, 12, 6));
		STPMissionChoices::AddRewards(WidgetTree, Card, Definition, ObjectiveSubsystem, 12);
		UTextBlock* Select = WidgetTree->ConstructWidget<UTextBlock>();
		Select->SetText(NSLOCTEXT("SurviveThePlanet", "ConfirmSingleObjective", "VÄLJ"));
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
