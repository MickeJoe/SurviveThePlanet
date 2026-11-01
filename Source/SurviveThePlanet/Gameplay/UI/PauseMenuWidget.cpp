#include "Gameplay/UI/PauseMenuWidget.h"
#include "Gameplay/UI/PlanetSelectorWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/PanelWidget.h"
#include "SurviveThePlanetPlayerController.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (SaveButton) SaveButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Save);
	if (LoadButton) LoadButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Load);
	if (SaveName) SaveName->SetText(FText::FromString(GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->GetSuggestedSaveName()));
	RefreshSaves();
	const bool bCanSave = GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->CanSaveCurrentWorld();
	if (SaveButton) SaveButton->SetIsEnabled(bCanSave);
	if (SaveName) SaveName->SetIsEnabled(bCanSave);
	if (SaveStatus) SaveStatus->SetText(GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->LastLoadStatus);
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Resume);
	PlanetSelectorButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::SelectPlanet);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Quit);
	if (PlanetSelectionContext)
	{
		if (auto* Header = Cast<UTextBlock>(GetWidgetFromName(TEXT("HeaderText")))) Header->SetText(FText::FromString(TEXT("MENU")));
		if (auto* Subtitle = Cast<UTextBlock>(GetWidgetFromName(TEXT("SubtitleText")))) Subtitle->SetText(FText::FromString(TEXT("Load a colony or return to planet selection")));
		if (auto* Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("ResumeButtonLabel")))) Label->SetText(FText::FromString(TEXT("BACK TO PLANET SELECTION")));
		if (auto* Hint = Cast<UTextBlock>(GetWidgetFromName(TEXT("ResumeHint")))) Hint->SetText(FText::FromString(TEXT("ESC   Back to planet selection")));
		if (PlanetSelectorButton->GetParent()) PlanetSelectorButton->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
		if (SaveStatus) SaveStatus->SetText(FText::FromString(TEXT("No active colony to save. Select a saved game above to load.")));
	}
}

FReply UPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		Resume();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UPauseMenuWidget::Resume()
{
	if (PlanetSelectionContext) { PlanetSelectionContext->ClosePauseMenu(); return; }
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->ResumeGame();
}

void UPauseMenuWidget::SelectPlanet()
{
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->ReturnToPlanetSelector();
}

void UPauseMenuWidget::Quit()
{
	if (PlanetSelectionContext) { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); return; }
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->QuitGame();
}

void UPauseMenuWidget::RefreshSaves()
{
	if (!SaveList) return;
	SaveList->ClearOptions(); SaveSlots.Reset();
	for (const auto& Entry : GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->ListSaves())
	{
		SaveSlots.Add(Entry.Slot);
		SaveList->AddOption(FString::Printf(TEXT("%d. %s | %s"), SaveSlots.Num(), *Entry.DisplayName,
			*FText::AsDateTime(Entry.SavedAt, EDateTimeStyle::Short, EDateTimeStyle::Short).ToString()));
	}
	if (!SaveSlots.IsEmpty()) SaveList->SetSelectedIndex(0);
	if (LoadButton) LoadButton->SetIsEnabled(!SaveSlots.IsEmpty());
	if (SaveStatus && SaveSlots.IsEmpty()) SaveStatus->SetText(FText::FromString(TEXT("No saved games yet. Every save creates a new slot.")));
}
void UPauseMenuWidget::Save()
{
	FText Status;
	const bool Success = GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->SaveColony(SaveName ? SaveName->GetText().ToString() : FString(), Status);
	if (Success) RefreshSaves();
	if (SaveStatus) SaveStatus->SetText(Status);
}
void UPauseMenuWidget::Load()
{
	const int32 Index = SaveList ? SaveList->GetSelectedIndex() : INDEX_NONE;
	if (!SaveSlots.IsValidIndex(Index)) return;
	FText Status;
	GetGameInstance()->GetSubsystem<UColonySaveSubsystem>()->LoadColony(SaveSlots[Index], Status);
	if (SaveStatus) SaveStatus->SetText(Status);
}
