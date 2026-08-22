#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Resources/ResourceManager.h"
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
	TObjectPtr<UTextBlock> FeedbackText;

	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void GiveSelectedResource();

private:
	void BuildFallbackLayout();
	void PopulateResources();
	TArray<EResourceType> ResourceTypes;
};
