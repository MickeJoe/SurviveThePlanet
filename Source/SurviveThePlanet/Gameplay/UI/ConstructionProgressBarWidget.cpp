#include "Gameplay/UI/ConstructionProgressBarWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Notifications/SProgressBar.h"

TSharedRef<SWidget> UConstructionProgressBarWidget::RebuildWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.85f))
		.Padding(FMargin(2.0f))
		[
			SAssignNew(ProgressBar, SProgressBar)
			.Percent(Progress)
			.FillColorAndOpacity(FLinearColor(0.1f, 0.78f, 0.45f, 1.0f))
		];
}

void UConstructionProgressBarWidget::SetProgress(float NewProgress)
{
	Progress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	SetVisibility(Progress >= 1.0f ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (ProgressBar.IsValid())
	{
		ProgressBar->SetPercent(Progress);
	}
}
