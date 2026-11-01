#include "Gameplay/UI/MissionChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "Gameplay/Resources/ResourceManager.h"
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
		case ESTPObjectiveRewardType::GiveDrone:
			return FText::FromString(FString::Printf(TEXT("+%d %s"), Reward.Amount,
				Reward.DroneClass.ToString().Contains(TEXT("Explorer")) ? TEXT("Explorer Drone") : TEXT("Worker Drone")));
		case ESTPObjectiveRewardType::GiveBuildingBlueprint:
			return FText::FromString(TEXT("BLUEPRINT: ") + Reward.BlueprintId.ToString());
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
	if (!OwnerWidget) return;
	if (bPacketNavigation) OwnerWidget->SelectPacket(PacketId);
	else OwnerWidget->HandleCandidateClicked(SlotIndex, ObjectiveId);
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
  if (auto* Button = Cast<UMissionChoiceButton>(Widget); Button && Button->GetSlotIndex() == INDEX_NONE && !Button->GetObjectiveId().IsNone())
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


void UMissionChoiceButton::InitializePacket(UMissionChoiceWidget* InOwner, FName InPacketId)
{
	OwnerWidget = InOwner;
	PacketId = InPacketId;
	bPacketNavigation = true;
	OnClicked.AddUniqueDynamic(this, &UMissionChoiceButton::HandleClicked);
}

void UMissionChoiceWidget::SelectPacket(FName PacketId)
{
	SelectedPacketId = PacketId;
	BuildCurrentGroup();
}

void UMissionChoiceWidget::BuildCurrentGroup()
{
	ChoiceColumns->ClearChildren();
	if (!ObjectiveSubsystem) return;
	HeaderText->SetVisibility(ESlateVisibility::Collapsed);
	DescriptionText->SetVisibility(ESlateVisibility::Collapsed);
	ChoiceStatusText->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmChoicesButton->SetVisibility(ESlateVisibility::Collapsed);
	const auto Groups = ObjectiveSubsystem->GetMissionChoiceGroups();
	if (SelectedPacketId.IsNone())
	{
		for (const auto& Group : Groups)
		{
			FSTPMissionChoiceGroupState State;
			ObjectiveSubsystem->GetMissionChoiceGroupState(Group.Id, State);
			if (!State.bLocked && ObjectiveSubsystem->IsMissionChoiceGroupUnlocked(Group.Id))
			{
				SelectedPacketId = Group.Id;
				break;
			}
		}
		if (SelectedPacketId.IsNone() && !Groups.IsEmpty()) SelectedPacketId = Groups[0].Id;
	}
	const auto* Selected = Groups.FindByPredicate([this](const auto& Group) { return Group.Id == SelectedPacketId; });
	CurrentGroup = Selected ? *Selected : FSTPMissionChoiceGroupDefinition();

	auto* Layout = WidgetTree->ConstructWidget<UVerticalBox>();
	auto* Container = ChoiceColumns->AddChildToHorizontalBox(Layout);
	Container->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Container->SetHorizontalAlignment(HAlign_Fill);
	Container->SetVerticalAlignment(VAlign_Fill);
	auto AddHeading = [this, Layout](const FString& Value, int32 Size, FLinearColor Color)
	{
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Text, Size, Color);
		Layout->AddChildToVerticalBox(Text)->SetPadding(FMargin(8, 3));
	};
	AddHeading(TEXT("MISSION OBJECTIVES  |  GAME PAUSED"), 20, STPMissionChoices::White);
	auto* Timeline = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Timeline->SetSlotPadding(FMargin(4));
	Layout->AddChildToVerticalBox(Timeline);
	int32 Index = 0;
	for (const auto& Group : Groups)
	{
		FSTPMissionChoiceGroupState State;
		ObjectiveSubsystem->GetMissionChoiceGroupState(Group.Id, State);
		const bool bUnlocked = ObjectiveSubsystem->IsMissionChoiceGroupUnlocked(Group.Id);
		FString Status;
		if (State.bLocked)
		{
			bool bCompleted = true;
			for (FName Id : State.SelectedObjectiveIds)
			{
				FSTPObjectiveRuntimeState Runtime;
				bCompleted &= ObjectiveSubsystem->GetObjectiveState(Id, Runtime) && Runtime.State == ESTPObjectiveState::Completed;
			}
			Status = bCompleted ? TEXT("COMPLETED") : TEXT("ACTIVE");
		}
		else if (bUnlocked) Status = TEXT("CHOOSE NOW");
		else Status = FString::Printf(TEXT("LOCKED | DAY %d %02d:%02d"), Group.UnlockDay, Group.UnlockMinuteOfDay / 60, Group.UnlockMinuteOfDay % 60);
		auto* Button = WidgetTree->ConstructWidget<UMissionChoiceButton>();
		Button->InitializePacket(this, Group.Id);
		Button->SetBackgroundColor(Group.Id == SelectedPacketId ? FLinearColor(0.05f, 0.35f, 0.42f) : STPMissionChoices::DefaultCard);
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(FString::Printf(TEXT("%02d\n%s"), Index + 1, *Status)));
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Label, 11, bUnlocked || State.bLocked ? STPMissionChoices::Cyan : FLinearColor(0.55f, 0.61f, 0.65f));
		Button->SetContent(Label);
		Timeline->AddChildToUniformGrid(STPMissionChoices::SizeCard(WidgetTree, Button, 166, 66), Index / 5, Index % 5);
		++Index;
	}
	// Tutorial and pre-packet saved requests remain accessible without occupying a packet.
	TSet<FName> PacketObjectives;
	for (const auto& Group : Groups) for (const auto& ChoiceSlot : Group.Slots) PacketObjectives.Append(ChoiceSlot.ObjectiveIds);
	TArray<FSTPObjectiveRuntimeState> OtherObjectives;
	for (const auto& Runtime : ObjectiveSubsystem->GetActiveObjectives())
		if (!PacketObjectives.Contains(Runtime.ObjectiveId)) OtherObjectives.Add(Runtime);
	if (!OtherObjectives.IsEmpty())
	{
		auto* Other = WidgetTree->ConstructWidget<UMissionChoiceButton>();
		Other->InitializePacket(this, TEXT("other"));
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(TEXT("OTHER ACTIVE MISSIONS")));
		STPMissionChoices::StyleText(Label, 11, STPMissionChoices::Gold);
		Other->SetContent(Label);
		Layout->AddChildToVerticalBox(Other)->SetPadding(FMargin(8, 3));
	}

	auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	auto* Cards = WidgetTree->ConstructWidget<UUniformGridPanel>();
	Cards->SetSlotPadding(FMargin(8));
	Scroll->AddChild(Cards);
	if (!Selected)
	{
		AddHeading(TEXT("ACTIVE MISSIONS"), 16, STPMissionChoices::Cyan);
		int32 CardIndex = 0;
		for (const auto& Runtime : OtherObjectives)
		{
			FSTPObjectiveDefinition Definition;
			if (ObjectiveSubsystem->GetObjectiveDefinition(Runtime.ObjectiveId, Definition))
			{
				Cards->AddChildToUniformGrid(BuildObjectiveCard(Definition, &Runtime, false), CardIndex / 2, CardIndex % 2);
				++CardIndex;
			}
		}
	}
	else
	{
		FSTPMissionChoiceGroupState State;
		ObjectiveSubsystem->GetMissionChoiceGroupState(Selected->Id, State);
		if (!State.bLocked && !ObjectiveSubsystem->IsMissionChoiceGroupUnlocked(Selected->Id))
		{
			const int32 Remaining = FMath::Max(0, FMath::CeilToInt(ObjectiveSubsystem->GetMissionUnlockMinutes(*Selected) - ObjectiveSubsystem->GetMissionGameMinutes()));
			AddHeading(FString::Printf(TEXT("LOCKED | DAY %d %02d:%02d | IN %dh %02dm"), Selected->UnlockDay,
				Selected->UnlockMinuteOfDay / 60, Selected->UnlockMinuteOfDay % 60, Remaining / 60, Remaining % 60), 16, STPMissionChoices::Gold);
			AddHeading(TEXT("Requires a completed Cargo Bay. Unlocks with game time; earlier missions do not need to be completed."), 13, STPMissionChoices::White);
		}
		else if (!State.bLocked)
		{
			AddHeading(Selected->Title.ToString() + TEXT(" | CHOOSE 1 OF 2"), 16, STPMissionChoices::Cyan);
			AddHeading(TEXT("Choose one mission. The other option is discarded. No deadline."), 12, STPMissionChoices::White);
			int32 CardIndex = 0;
			for (FName Id : Selected->Slots[0].ObjectiveIds)
			{
				FSTPObjectiveDefinition Definition;
				if (ObjectiveSubsystem->GetObjectiveDefinition(Id, Definition))
					Cards->AddChildToUniformGrid(BuildObjectiveCard(Definition, nullptr, true), 0, CardIndex++);
			}
		}
		else
		{
			AddHeading(Selected->Title.ToString() + TEXT(" | ACCEPTED MISSION"), 16, STPMissionChoices::Cyan);
			for (FName Id : State.SelectedObjectiveIds)
			{
				FSTPObjectiveDefinition Definition; FSTPObjectiveRuntimeState Runtime;
				if (ObjectiveSubsystem->GetObjectiveDefinition(Id, Definition) && ObjectiveSubsystem->GetObjectiveState(Id, Runtime))
					Cards->AddChildToUniformGrid(BuildObjectiveCard(Definition, &Runtime, false), 0, 0);
			}
		}
	}
	Layout->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}

UWidget* UMissionChoiceWidget::BuildObjectiveCard(const FSTPObjectiveDefinition& Definition,
	const FSTPObjectiveRuntimeState* Runtime, bool bCandidate)
{
	auto* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(STPMissionChoices::DefaultCard);
	auto* Card = WidgetTree->ConstructWidget<UVerticalBox>();
	Border->SetContent(Card);
	auto AddLine = [this, Card](const FString& Value, int32 Size, FLinearColor Color)
	{
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetAutoWrapText(true);
		STPMissionChoices::StyleText(Text, Size, Color);
		Card->AddChildToVerticalBox(Text)->SetPadding(FMargin(12, 4));
	};
	AddLine(Definition.Title.ToString(), 17, STPMissionChoices::Cyan);
	AddLine(Definition.Description.ToString(), 12, STPMissionChoices::White);
	const bool bCompleted = Runtime && Runtime->State == ESTPObjectiveState::Completed;
	if (bCompleted) AddLine(TEXT("MISSION COMPLETED"), 15, FLinearColor(0.5f, 0.85f, 0.3f));
	for (int32 ConditionIndex = 0; ConditionIndex < Definition.Conditions.Num(); ++ConditionIndex)
	{
		const auto& Condition = Definition.Conditions[ConditionIndex];
		const int32 Progress = Runtime && Runtime->ConditionProgress.IsValidIndex(ConditionIndex) ? Runtime->ConditionProgress[ConditionIndex] : 0;
		if (Condition.Type == ESTPObjectiveConditionType::DeliverResource && !bCompleted)
		{
			int32 Stock = 0;
			for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It) { Stock = It->GetResourceAmount(Condition.ResourceType); break; }
			const int32 Remaining = FMath::Max(0, Condition.RequiredAmount - Progress);
			AddLine(FString::Printf(TEXT("In storage: %d / %d"), Stock, Remaining), 14, STPMissionChoices::White);
			AddLine(FString::Printf(TEXT("Missing: %d"), FMath::Max(0, Remaining - Stock)), 12, STPMissionChoices::Gold);
		}
		if (Runtime) AddLine(FString::Printf(TEXT("PROGRESS %d / %d"), Progress, Condition.RequiredAmount), 12, STPMissionChoices::Gold);
	}
	STPMissionChoices::AddRewards(WidgetTree, Card, Definition, ObjectiveSubsystem, 12);
	const bool bDelivery = !Definition.Conditions.IsEmpty() && Definition.Conditions[0].Type == ESTPObjectiveConditionType::DeliverResource;
	if (bCandidate || (bDelivery && Runtime && Runtime->State == ESTPObjectiveState::Active))
	{
		auto* Button = WidgetTree->ConstructWidget<UMissionChoiceButton>();
		Button->InitializeChoice(this, bCandidate ? 0 : INDEX_NONE, Definition.Id);
		Button->SetIsEnabled(bCandidate || ObjectiveSubsystem->CanDeliverObjective(Definition.Id));
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(bCandidate ? TEXT("ACCEPT MISSION") : TEXT("DELIVER TO MOTHER BASE")));
		Label->SetJustification(ETextJustify::Center);
		STPMissionChoices::StyleText(Label, 13, STPMissionChoices::Cyan);
		Button->SetContent(Label);
		Card->AddChildToVerticalBox(Button)->SetPadding(FMargin(12, 10));
	}
	return STPMissionChoices::SizeCard(WidgetTree, Border, 390, 310);
}

void UMissionChoiceWidget::HandleCandidateClicked(int32 SlotIndex, FName ObjectiveId)
{
	if (!ObjectiveSubsystem || ObjectiveId.IsNone()) return;
	if (SlotIndex == INDEX_NONE) ObjectiveSubsystem->DeliverObjective(ObjectiveId);
	else ObjectiveSubsystem->ConfirmMissionChoiceSlot(CurrentGroup.Id, SlotIndex, ObjectiveId);
	BuildCurrentGroup();
}

void UMissionChoiceWidget::HandleGroupConfirmed(FName ChoiceGroupId)
{
	if (ChoiceGroupId == CurrentGroup.Id) BuildCurrentGroup();
}
