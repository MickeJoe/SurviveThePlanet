#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "MissionResultWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/** Presentation and rewards for WBP_MissionResult. Mission rules live in the subsystem. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UMissionResultWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void PresentResult(EMissionOutcome Outcome);
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(EditDefaultsOnly, Category="Rewards", meta=(ClampMin="0")) int32 VictoryCredits = 2500;
	UPROPERTY(EditDefaultsOnly, Category="Rewards", meta=(ClampMin="0")) int32 DefeatCredits = 450;
	UPROPERTY(EditDefaultsOnly, Category="Rewards") TArray<FResourceCost> VictoryResources;
	UPROPERTY(EditDefaultsOnly, Category="Rewards") TArray<FResourceCost> DefeatResources;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> AccentBorder;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> OutcomeText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ReasonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RewardsHeading;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CreditsRewardText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ResourceRewardText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProgressValue;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BuildingsValue;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TimeValue;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ConfidenceValue;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PlanetSelectButton;
private:
	bool bPresented = false;
	UFUNCTION() void Retry();
	UFUNCTION() void SelectPlanet();
};
