#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "ObjectiveTrackerWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** Native behavior for WBP_ObjectiveTracker. The Widget Blueprint owns the layout. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UObjectiveTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

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
	static FText FormatProgress(const FSTPObjectiveDefinition& Definition, const FSTPObjectiveRuntimeState& State);
	static FText FormatTime(float RemainingSeconds);
};
