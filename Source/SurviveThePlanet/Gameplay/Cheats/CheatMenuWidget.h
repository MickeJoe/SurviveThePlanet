#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "CheatMenuWidget.generated.h"

class UButton;
class UComboBoxString;
class USpinBox;
class UTextBlock;

/** Development-only UMG cheat panel. May be subclassed by WBP_CheatMenu for visual iteration. */
UCLASS()
class SURVIVETHEPLANET_API UCheatMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResourceComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> AmountSpinBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> GiveButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CompleteObjectiveButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> BlueprintComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> GrantBlueprintButton;

	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void GiveSelectedResource();

	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void CompleteUplinkObjective();

	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void GrantSelectedBlueprint();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OpenTradeButton;
	UFUNCTION() void OpenTrade();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Speed10Button;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Speed30Button;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfidenceDecayButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfidenceDecayButtonLabel;
	UFUNCTION()
	void EnableSpeed10();
	UFUNCTION()
	void EnableSpeed30();
	UFUNCTION()
	void ToggleConfidenceDecay();

private:
	void EnableCheatSpeed(float Speed);
	void RefreshConfidenceDecayLabel();
	void BuildFallbackLayout();
	void PopulateResources();
	void EnsureObjectiveCheatButton();
	void EnsureBlueprintCheatControls();
	TArray<EResourceType> ResourceTypes;
	TArray<ESTPBuildTool> BlueprintTools;
};
