#include "Gameplay/UI/MissionResultWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Kismet/GameplayStatics.h"
#include "SurviveThePlanetPlayerController.h"

void UMissionResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RetryButton->OnClicked.AddUniqueDynamic(this, &UMissionResultWidget::Retry);
	PlanetSelectButton->OnClicked.AddUniqueDynamic(this, &UMissionResultWidget::SelectPlanet);
}

void UMissionResultWidget::PresentResult(EMissionOutcome Outcome)
{
	if (bPresented || Outcome == EMissionOutcome::InProgress) return;
	bPresented = true;
	const bool bVictory = Outcome == EMissionOutcome::Complete;
	const FLinearColor Accent = bVictory ? FLinearColor(0.0f, 0.72f, 0.9f) : FLinearColor(1.0f, 0.19f, 0.035f);
	AccentBorder->SetBrushColor(Accent);
	OutcomeText->SetText(FText::FromString(bVictory ? TEXT("MISSION COMPLETE") : TEXT("MISSION FAILED")));
	OutcomeText->SetColorAndOpacity(FSlateColor(Accent));
	ReasonText->SetText(FText::FromString(bVictory ? TEXT("Mission progression complete") : TEXT("Mission confidence depleted")));
	RewardsHeading->SetText(FText::FromString(bVictory ? TEXT("REWARDS") : TEXT("RECOVERED REWARDS")));
	const int32 Credits = FMath::Max(0, bVictory ? VictoryCredits : DefeatCredits);
	const TArray<FResourceCost>& Resources = bVictory ? VictoryResources : DefeatResources;
	AResourceManager* Manager = nullptr;
	for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
	CreditsRewardText->SetText(FText::FromString(Manager ? FString::Printf(TEXT("+%s  CREDITS"), *FText::AsNumber(Credits).ToString()) : TEXT("No rewards available")));
	FString ResourceSummary;
	if (Manager)
	{
		Manager->AddCredits(Credits);
		for (const FResourceCost& Resource : Resources)
		{
			if (Resource.Cost <= 0) continue;
			Manager->AddResource(Resource.Resource, Resource.Cost);
			if (!ResourceSummary.IsEmpty()) ResourceSummary += TEXT("\n");
			ResourceSummary += FString::Printf(TEXT("+%d  %s"), Resource.Cost,
				*StaticEnum<EResourceType>()->GetDisplayNameTextByValue(static_cast<int64>(Resource.Resource)).ToString());
		}
	}
	ResourceRewardText->SetText(FText::FromString(ResourceSummary));
	int32 Buildings = 0;
	for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
	{
		if (!It->IsPlacementPreview() && !It->IsActorBeingDestroyed() && It->GetConstructionProgress() >= 1.0f) ++Buildings;
	}
	BuildingsValue->SetText(FText::AsNumber(Buildings));
	if (const auto* Mission = GetWorld()->GetSubsystem<UMissionConfidenceSubsystem>())
	{
		ProgressValue->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Mission->GetMissionProgress(), Mission->GetMissionProgressTarget())));
		ConfidenceValue->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), Mission->GetMissionConfidence())));
		const int32 Seconds = FMath::FloorToInt(Mission->GetMissionElapsedSeconds());
		TimeValue->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), Seconds / 3600, (Seconds / 60) % 60, Seconds % 60)));
	}
}

void UMissionResultWidget::Retry()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void UMissionResultWidget::SelectPlanet()
{
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->ReturnToPlanetSelector();
}
