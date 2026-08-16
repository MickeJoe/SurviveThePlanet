#include "Gameplay/UI/ObjectiveTrackerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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
	for (int32 Index = 0; Index < ActiveObjectives.Num() && Index < 8; ++Index)
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
	Title->SetClipping(EWidgetClipping::ClipToBounds);
	STPObjectiveUI::StyleText(Title, 15, bPrimary ? STPObjectiveUI::MainText : STPObjectiveUI::SecondaryText);

	FString StatusText = FormatProgress(Definition, State).ToString();
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

	UVerticalBoxSlot* RowSlot = ObjectiveList->AddChildToVerticalBox(Row);
	RowSlot->SetPadding(FMargin(0.0f, 1.0f));
	RowSlot->SetHorizontalAlignment(HAlign_Fill);
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
