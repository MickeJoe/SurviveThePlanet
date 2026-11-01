#include "Gameplay/UI/ObjectiveTrackerWidget.h"
#include "Gameplay/UI/WorldHoverTooltipWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Engine/GameInstance.h"
#include "Gameplay/Resources/ResourceCatalog.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Drones/BaseDrone.h"

namespace STPObjectiveUI
{
	// High-contrast text is intentional: the tracker has no backing panel or gradient.
	const FLinearColor MainText(0.92f, 0.94f, 0.94f, 1.0f);
	const FLinearColor SecondaryText(0.74f, 0.78f, 0.79f, 1.0f);
	const FLinearColor Cyan(0.10f, 0.72f, 0.94f, 1.0f);
	const FLinearColor Urgent(1.0f, 0.50f, 0.16f, 1.0f);
	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.95f);

	void StyleText(UTextBlock* Text, int32 Size, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = Shadow;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(Shadow);
	}
}

void UObjectiveRewardIconWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (RewardDescription.IsEmpty() || !GetOwningPlayer()) return;
	HideRewardTooltip();
	UClass* Class = LoadClass<UWorldHoverTooltipWidget>(nullptr,
		TEXT("/Game/UI/WBP_WorldHoverTooltip.WBP_WorldHoverTooltip_C"));
	if (!Class) return;
	HoverTooltip = CreateWidget<UWorldHoverTooltipWidget>(GetOwningPlayer(), Class);
	if (!HoverTooltip) return;
	HoverTooltip->SetDisplayText(RewardDescription, 260.0f);
	HoverTooltip->AddToViewport(100);
	HoverTooltip->ForceLayoutPrepass();
	float MouseX = 0.0f, MouseY = 0.0f;
	GetOwningPlayer()->GetMousePosition(MouseX, MouseY);
	int32 Width = 0, Height = 0;
	GetOwningPlayer()->GetViewportSize(Width, Height);
	const FVector2D Size = HoverTooltip->GetDesiredSize() * UWidgetLayoutLibrary::GetViewportScale(this);
	HoverTooltip->SetPositionInViewport(FVector2D(
		FMath::Clamp(MouseX + 16.0f, 0.0f, FMath::Max(0.0f, Width - Size.X - 8.0f)),
		FMath::Clamp(MouseY + 22.0f, 0.0f, FMath::Max(0.0f, Height - Size.Y - 8.0f))));
}

void UObjectiveRewardIconWidget::HideRewardTooltip()
{
	if (HoverTooltip) HoverTooltip->RemoveFromParent();
	HoverTooltip = nullptr;
}

void UObjectiveRewardIconWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	HideRewardTooltip();
	Super::NativeOnMouseLeave(Event);
}

void UObjectiveRewardIconWidget::NativeDestruct()
{
	HideRewardTooltip();
	Super::NativeDestruct();
}

void UObjectiveTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ObjectivesHeading)
	{
		ObjectivesHeading->SetText(NSLOCTEXT("SurviveThePlanet", "ObjectivesHeading", "OBJECTIVES"));
		STPObjectiveUI::StyleText(ObjectivesHeading, 20, STPObjectiveUI::MainText);
	}

	ObjectiveSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UObjectiveSubsystem>() : nullptr;
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnObjectiveStateChanged.AddUniqueDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveStateChanged);
		ObjectiveSubsystem->OnObjectiveProgressChanged.AddUniqueDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveProgressChanged);
		ObjectiveSubsystem->OnObjectiveTimeChanged.AddUniqueDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveTimeChanged);
	}
	RefreshObjectives();
}

void UObjectiveTrackerWidget::NativeDestruct()
{
	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnObjectiveStateChanged.RemoveDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveStateChanged);
		ObjectiveSubsystem->OnObjectiveProgressChanged.RemoveDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveProgressChanged);
		ObjectiveSubsystem->OnObjectiveTimeChanged.RemoveDynamic(this, &UObjectiveTrackerWidget::HandleObjectiveTimeChanged);
	}
	ObjectiveSubsystem = nullptr;
	Super::NativeDestruct();
}

void UObjectiveTrackerWidget::HandleObjectiveStateChanged(FName ObjectiveId, ESTPObjectiveState NewState)
{
	RefreshObjectives();
}

void UObjectiveTrackerWidget::HandleObjectiveProgressChanged(FName ObjectiveId, int32 ConditionIndex, int32 CurrentAmount, int32 RequiredAmount)
{
	RefreshObjectives();
}

void UObjectiveTrackerWidget::HandleObjectiveTimeChanged(FName ObjectiveId, float RemainingSeconds)
{
	RefreshObjectives();
}

void UObjectiveTrackerWidget::RefreshObjectives()
{
	if (!ObjectiveList)
	{
		return;
	}
	ObjectiveList->ClearChildren();
	if (!ObjectiveSubsystem)
	{
		return;
	}

	const TArray<FSTPObjectiveRuntimeState> ActiveObjectives = ObjectiveSubsystem->GetActiveObjectives();
	for (int32 Index = 0; Index < ActiveObjectives.Num() && Index < 10; ++Index)
	{
		FSTPObjectiveDefinition Definition;
		if (ObjectiveSubsystem->GetObjectiveDefinition(ActiveObjectives[Index].ObjectiveId, Definition))
		{
			AddObjectiveRow(Definition, ActiveObjectives[Index], Index == 0);
		}
	}
}

void UObjectiveTrackerWidget::AddObjectiveRow(const FSTPObjectiveDefinition& Definition,
	const FSTPObjectiveRuntimeState& State, bool bPrimary)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	UTextBlock* Marker = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	UTextBlock* Progress = WidgetTree->ConstructWidget<UTextBlock>();

	Marker->SetText(bPrimary ? FText::FromString(TEXT("◆")) : FText::GetEmpty());
	STPObjectiveUI::StyleText(Marker, 11, STPObjectiveUI::Cyan);
	Title->SetText(Definition.Title);
	Title->SetAutoWrapText(true);
	STPObjectiveUI::StyleText(Title, 15, bPrimary ? STPObjectiveUI::MainText : STPObjectiveUI::SecondaryText);

	const bool bDelivery = Definition.Conditions.ContainsByPredicate([](const auto& Condition)
	{
		return Condition.Type == ESTPObjectiveConditionType::DeliverResource;
	});
	FString StatusText = bDelivery ? FString() : FormatProgress(Definition, State).ToString();
	if (State.RemainingTimeSeconds >= 0.0f)
	{
		StatusText += FString::Printf(TEXT("   %s"), *FormatTime(State.RemainingTimeSeconds).ToString());
	}
	Progress->SetText(FText::FromString(StatusText));
	Progress->SetJustification(ETextJustify::Right);
	const bool bUrgent = State.RemainingTimeSeconds >= 0.0f && State.RemainingTimeSeconds <= 60.0f;
	STPObjectiveUI::StyleText(Progress, 15, bUrgent ? STPObjectiveUI::Urgent : STPObjectiveUI::Cyan);

	UHorizontalBoxSlot* MarkerSlot = Row->AddChildToHorizontalBox(Marker);
	MarkerSlot->SetPadding(FMargin(0.0f, 2.0f, 7.0f, 2.0f));
	MarkerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(Title);
	TitleSlot->SetPadding(FMargin(0.0f, 2.0f, 12.0f, 2.0f));
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UHorizontalBoxSlot* ProgressSlot = Row->AddChildToHorizontalBox(Progress);
	ProgressSlot->SetPadding(FMargin(0.0f, 2.0f));
	ProgressSlot->SetHorizontalAlignment(HAlign_Right);
	ProgressSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* ObjectiveCard = WidgetTree->ConstructWidget<UVerticalBox>();
	ObjectiveCard->AddChildToVerticalBox(Row);
	for (int32 ConditionIndex = 0; ConditionIndex < Definition.Conditions.Num(); ++ConditionIndex)
	{
		const auto& Condition = Definition.Conditions[ConditionIndex];
		if (Condition.Type != ESTPObjectiveConditionType::DeliverResource) continue;
		FText ResourceName = StaticEnum<EResourceType>()->GetDisplayNameTextByValue(static_cast<int64>(Condition.ResourceType));
		for (const auto& Resource : UResourceCatalog::GetDefinitions())
			if (Resource.ResourceType == Condition.ResourceType) { ResourceName = Resource.DisplayName; break; }
		auto* DeliveryRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto* Icon = WidgetTree->ConstructWidget<UImage>();
		Icon->SetBrushFromTexture(UResourceCatalog::GetResourceIcon(Condition.ResourceType));
		auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
		IconSize->SetWidthOverride(20); IconSize->SetHeightOverride(20); IconSize->SetContent(Icon);
		DeliveryRow->AddChildToHorizontalBox(IconSize)->SetPadding(FMargin(0, 0, 6, 0));
		auto* ResourceText = WidgetTree->ConstructWidget<UTextBlock>();
		ResourceText->SetText(FText::Format(NSLOCTEXT("STPObjectives", "DeliveryRequirement", "Deliver {0}"), ResourceName));
		ResourceText->SetAutoWrapText(true);
		STPObjectiveUI::StyleText(ResourceText, 13, STPObjectiveUI::MainText);
		DeliveryRow->AddChildToHorizontalBox(ResourceText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		auto* Amount = WidgetTree->ConstructWidget<UTextBlock>();
		const int32 Delivered = State.ConditionProgress.IsValidIndex(ConditionIndex) ? State.ConditionProgress[ConditionIndex] : 0;
		Amount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Delivered, Condition.RequiredAmount)));
		STPObjectiveUI::StyleText(Amount, 13, STPObjectiveUI::Cyan);
		DeliveryRow->AddChildToHorizontalBox(Amount)->SetPadding(FMargin(6, 0, 0, 0));
		ObjectiveCard->AddChildToVerticalBox(DeliveryRow)->SetPadding(FMargin(18, 1, 0, 2));
	}
	AddRewardIcons(ObjectiveCard, Definition);

	UVerticalBoxSlot* RowSlot = ObjectiveList->AddChildToVerticalBox(ObjectiveCard);
	RowSlot->SetPadding(FMargin(0.0f, 1.0f));
	RowSlot->SetHorizontalAlignment(HAlign_Fill);
}

void UObjectiveTrackerWidget::AddRewardIcons(UVerticalBox* ObjectiveCard, const FSTPObjectiveDefinition& Definition)
{
	if (Definition.Rewards.IsEmpty()) return;
	UClass* StripClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/Objectives/WBP_ObjectiveRewardStrip.WBP_ObjectiveRewardStrip_C"));
	UClass* IconClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/UI/Objectives/WBP_ObjectiveRewardIcon.WBP_ObjectiveRewardIcon_C"));
	if (!StripClass || !IconClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Objective reward Widget Blueprints are missing."));
		return;
	}
	UUserWidget* Strip = CreateWidget<UUserWidget>(GetOwningPlayer(), StripClass);
	UWrapBox* RewardsWrap = Strip ? Cast<UWrapBox>(Strip->GetWidgetFromName(TEXT("RewardsWrap"))) : nullptr;
	if (!RewardsWrap) return;
	Strip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ObjectiveCard->AddChildToVerticalBox(Strip)->SetPadding(FMargin(18.0f, 2.0f, 0.0f, 5.0f));
	const UBuildingBlueprintSubsystem* Blueprints = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>() : nullptr;

	for (const FSTPObjectiveRewardDefinition& Reward : Definition.Rewards)
	{
		UTexture2D* Texture = nullptr;
		FString Amount;
		FText Description;
		const bool bBlueprint = Reward.Type == ESTPObjectiveRewardType::GiveBuildingBlueprint;
		switch (Reward.Type)
		{
		case ESTPObjectiveRewardType::GiveResource:
		{
			Texture = UResourceCatalog::GetResourceIcon(Reward.ResourceType);
			FText Name = StaticEnum<EResourceType>()->GetDisplayNameTextByValue(static_cast<int64>(Reward.ResourceType));
			for (const FResourceDefinition& Resource : UResourceCatalog::GetDefinitions())
				if (Resource.ResourceType == Reward.ResourceType) { Name = Resource.DisplayName; break; }
			Amount = FString::Printf(TEXT("x%d"), Reward.Amount);
			Description = FText::Format(NSLOCTEXT("STPRewards", "Resource", "{0} {1}"), FText::AsNumber(Reward.Amount), Name);
			break;
		}
		case ESTPObjectiveRewardType::GiveMissionConfidence:
			Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Objectives/T_RewardConfidence.T_RewardConfidence"));
			Amount = FString::Printf(TEXT("+%d"), Reward.Amount);
			Description = FText::Format(NSLOCTEXT("STPRewards", "Confidence", "+{0} percentage points of mission confidence (maximum 100%)."), FText::AsNumber(Reward.Amount));
			break;
		case ESTPObjectiveRewardType::GiveMissionProgress:
			Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Objectives/T_RewardProgress.T_RewardProgress"));
			Amount = FString::Printf(TEXT("+%d"), Reward.Amount);
			Description = FText::Format(NSLOCTEXT("STPRewards", "Progress", "+{0} mission progression"), FText::AsNumber(Reward.Amount));
			break;
		case ESTPObjectiveRewardType::GiveDrone:
		{
			const UClass* DroneClass = Reward.DroneClass.TryLoadClass<ABaseDrone>();
			const ABaseDrone* Drone = DroneClass ? DroneClass->GetDefaultObject<ABaseDrone>() : nullptr;
			Texture = Drone ? Drone->GetDroneThumbnail() : nullptr;
			Amount = FString::Printf(TEXT("x%d"), Reward.Amount);
			Description = FText::Format(NSLOCTEXT("STPRewards", "Drone", "{0} x {1}"), FText::AsNumber(Reward.Amount),
				Drone ? Drone->GetDroneDisplayName() : FText::FromString(Reward.DroneClass.ToString()));
			break;
		}
		case ESTPObjectiveRewardType::GiveBuildingBlueprint:
		{
			const UBuildingDataAsset* Building = Blueprints ? Blueprints->FindDefinitionByBlueprintId(Reward.BlueprintId) : nullptr;
			Texture = Building ? (Building->ToolbarIcon ? Building->ToolbarIcon.Get() : Building->Thumbnail.Get()) : nullptr;
			Description = FText::Format(NSLOCTEXT("STPRewards", "Blueprint", "Blueprint: {0}\nUnlocks this building in the build menu."),
				Building ? Building->DisplayName : FText::FromName(Reward.BlueprintId));
			break;
		}
		case ESTPObjectiveRewardType::UnlockObjective:
		{
			Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/BuildingFlow/T_FlowArrow.T_FlowArrow"));
			FSTPObjectiveDefinition Next;
			Description = FText::Format(NSLOCTEXT("STPRewards", "NextObjective", "Next objective: {0}"),
				ObjectiveSubsystem && ObjectiveSubsystem->GetObjectiveDefinition(Reward.ObjectiveId, Next)
					? Next.Title : FText::FromName(Reward.ObjectiveId));
			break;
		}
		}
		// Keep a visible reward and its tooltip even when optional catalog art is missing.
		if (!Texture) Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Objectives/T_RewardBlueprint.T_RewardBlueprint"));
		UObjectiveRewardIconWidget* Card = CreateWidget<UObjectiveRewardIconWidget>(GetOwningPlayer(), IconClass);
		if (!Card) continue;
		Card->SetVisibility(ESlateVisibility::Visible);
		Card->SetRewardDescription(FText::Format(NSLOCTEXT("STPRewards", "Tooltip", "On completion\n{0}"), Description));
		if (UImage* Icon = Cast<UImage>(Card->GetWidgetFromName(TEXT("RewardIcon")))) Icon->SetBrushFromTexture(Texture);
		if (UTextBlock* Quantity = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("AmountText"))))
		{
			Quantity->SetText(FText::FromString(Amount));
			Quantity->SetVisibility(Amount.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		if (UWidget* Badge = Card->GetWidgetFromName(TEXT("BlueprintBadge")))
			Badge->SetVisibility(bBlueprint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		RewardsWrap->AddChildToWrapBox(Card);
	}
}

FText UObjectiveTrackerWidget::FormatProgress(const FSTPObjectiveDefinition& Definition,
	const FSTPObjectiveRuntimeState& State)
{
	if (Definition.Conditions.IsEmpty() || State.ConditionProgress.IsEmpty())
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(TEXT("%d / %d"),
		State.ConditionProgress[0], Definition.Conditions[0].RequiredAmount));
}

FText UObjectiveTrackerWidget::FormatTime(float RemainingSeconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60));
}

float UObjectiveTrackerWidget::GetObjectivesBottomInViewport() const
{
	if (!ObjectiveList || ObjectiveList->GetCachedGeometry().GetLocalSize().IsNearlyZero()) return 0.0f;
	FVector2D PixelPosition, ViewportPosition;
	USlateBlueprintLibrary::LocalToViewport(this, ObjectiveList->GetCachedGeometry(),
		FVector2D(0.0f, ObjectiveList->GetDesiredSize().Y), PixelPosition, ViewportPosition);
	return ViewportPosition.Y;
}
