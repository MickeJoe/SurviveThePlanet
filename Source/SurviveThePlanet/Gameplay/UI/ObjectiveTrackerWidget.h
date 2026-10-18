#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "ObjectiveTrackerWidget.generated.h"

class UTextBlock;
class UVerticalBox;

class UWorldHoverTooltipWidget;

/** Compact WBP reward tile with the same in-game tooltip used by world objects. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UObjectiveRewardIconWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetRewardDescription(const FText& Text) { RewardDescription = Text; }
protected:
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
	virtual void NativeDestruct() override;
private:
	UPROPERTY(Transient) FText RewardDescription;
	UPROPERTY(Transient) TObjectPtr<UWorldHoverTooltipWidget> HoverTooltip;
	void HideRewardTooltip();
};

/** Native behavior for WBP_ObjectiveTracker. The Widget Blueprint owns the layout. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UObjectiveTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	float GetObjectivesBottomInViewport() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Container authored in WBP_ObjectiveTracker. Runtime objective rows are inserted here. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Objectives")
	TObjectPtr<UVerticalBox> ObjectiveList;

	/** Optional heading authored in the WBP. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Objectives")
	TObjectPtr<UTextBlock> ObjectivesHeading;

private:
	UPROPERTY(Transient)
	TObjectPtr<UObjectiveSubsystem> ObjectiveSubsystem;

	UFUNCTION()
	void HandleObjectiveStateChanged(FName ObjectiveId, ESTPObjectiveState NewState);

	UFUNCTION()
	void HandleObjectiveProgressChanged(FName ObjectiveId, int32 ConditionIndex, int32 CurrentAmount, int32 RequiredAmount);

	UFUNCTION()
	void HandleObjectiveTimeChanged(FName ObjectiveId, float RemainingSeconds);

	void RefreshObjectives();
	void AddObjectiveRow(const FSTPObjectiveDefinition& Definition, const FSTPObjectiveRuntimeState& State, bool bPrimary);
	void AddRewardIcons(UVerticalBox* ObjectiveCard, const FSTPObjectiveDefinition& Definition);
	static FText FormatProgress(const FSTPObjectiveDefinition& Definition, const FSTPObjectiveRuntimeState& State);
	static FText FormatTime(float RemainingSeconds);
};
