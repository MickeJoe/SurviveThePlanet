#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UPlanetSelectorWidget;
class UEditableTextBox;
class UComboBoxString;
class UTextBlock;

/** Visual layout is authored in WBP_PauseMenu. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetPlanetSelectionContext(UPlanetSelectorWidget* Selector) { PlanetSelectionContext = Selector; }
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ResumeButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PlanetSelectorButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> SaveButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> LoadButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UEditableTextBox> SaveName;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UComboBoxString> SaveList;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SaveStatus;
private:
	UPROPERTY(Transient) TObjectPtr<UPlanetSelectorWidget> PlanetSelectionContext;
	TArray<FString> SaveSlots;
	void RefreshSaves();
	UFUNCTION() void Save();
	UFUNCTION() void Load();
	UFUNCTION() void Resume();
	UFUNCTION() void SelectPlanet();
	UFUNCTION() void Quit();
};
