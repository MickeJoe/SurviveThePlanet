#include "Gameplay/UI/PauseMenuWidget.h"
#include "SurviveThePlanetPlayerController.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Resume);
	PlanetSelectorButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::SelectPlanet);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UPauseMenuWidget::Quit);
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
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->ResumeGame();
}

void UPauseMenuWidget::SelectPlanet()
{
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->ReturnToPlanetSelector();
}

void UPauseMenuWidget::Quit()
{
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->QuitGame();
}
