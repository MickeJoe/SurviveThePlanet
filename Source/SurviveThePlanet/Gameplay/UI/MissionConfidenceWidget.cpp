#include "Gameplay/UI/MissionConfidenceWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"

void UMissionConfidenceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfidenceSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>() : nullptr;
	if (ConfidenceSubsystem)
	{
		ConfidenceSubsystem->OnMissionConfidenceChanged.AddUniqueDynamic(this, &UMissionConfidenceWidget::HandleConfidenceChanged);
		Refresh(ConfidenceSubsystem->GetMissionConfidence());
	}
}

void UMissionConfidenceWidget::NativeDestruct()
{
	if (ConfidenceSubsystem)
	{
		ConfidenceSubsystem->OnMissionConfidenceChanged.RemoveDynamic(this, &UMissionConfidenceWidget::HandleConfidenceChanged);
	}
	Super::NativeDestruct();
}

void UMissionConfidenceWidget::HandleConfidenceChanged(float NewConfidence, float Delta)
{
	Refresh(NewConfidence);
}

void UMissionConfidenceWidget::Refresh(float Confidence)
{
	if (ConfidencePercentText)
	{
		ConfidencePercentText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Confidence))));
	}
	if (ConfidenceDecayText && ConfidenceSubsystem)
	{
		ConfidenceDecayText->SetText(FText::FromString(FString::Printf(TEXT("-%.1f%% / h"), ConfidenceSubsystem->GetDecayPerGameHour())));
	}
	BuildSegments(Confidence);
}

void UMissionConfidenceWidget::BuildSegments(float Confidence)
{
	if (!ConfidenceSegments)
	{
		return;
	}
	ConfidenceSegments->ClearChildren();
	for (int32 Index = 0; Index < 10; ++Index)
	{
		USizeBox* Size = NewObject<USizeBox>(ConfidenceSegments);
		Size->SetWidthOverride(18.0f);
		Size->SetHeightOverride(14.0f);
		UBorder* Segment = NewObject<UBorder>(Size);
		const float SegmentFill = FMath::Clamp((Confidence - Index * 10.0f) / 10.0f, 0.0f, 1.0f);
		Segment->SetBrushColor(FLinearColor::LerpUsingHSV(
			FLinearColor(0.035f, 0.075f, 0.09f, 0.9f),
			FLinearColor(0.0f, 0.62f, 1.0f, 1.0f), SegmentFill));
		Size->SetContent(Segment);
		if (UHorizontalBoxSlot* SegmentSlot = ConfidenceSegments->AddChildToHorizontalBox(Size))
		{
			SegmentSlot->SetPadding(FMargin(0.0f, 0.0f, Index == 9 ? 0.0f : 3.0f, 0.0f));
		}
	}
}
