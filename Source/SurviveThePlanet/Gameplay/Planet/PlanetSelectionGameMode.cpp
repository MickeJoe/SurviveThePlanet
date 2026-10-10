#include "PlanetSelectionGameMode.h"
#include "Gameplay/UI/PlanetSelectorWidget.h"
#include "GameFramework/PlayerController.h"
APlanetSelectionGameMode::APlanetSelectionGameMode() { DefaultPawnClass = nullptr; }
void APlanetSelectionGameMode::StartPlay()
{
	Super::StartPlay();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;
	auto Class = LoadClass<UPlanetSelectorWidget>(nullptr, TEXT("/Game/UI/Planet/WBP_PlanetSelector.WBP_PlanetSelector_C"));
	if (!Class) return;
	UPlanetSelectorWidget* Widget = CreateWidget<UPlanetSelectorWidget>(PC, Class);
	Widget->AddToViewport();
	PC->bShowMouseCursor = true;
	FInputModeUIOnly Input;
	Input.SetWidgetToFocus(Widget->TakeWidget());
	PC->SetInputMode(Input);
}
