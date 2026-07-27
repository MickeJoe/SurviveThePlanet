// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurviveThePlanetGameMode.h"

#include "SurviveThePlanetCameraPawn.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/UI/BuildToolbarWidget.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "SurviveThePlanet.h"

ASurviveThePlanetGameMode::ASurviveThePlanetGameMode()
{
	DefaultPawnClass = ASurviveThePlanetCameraPawn::StaticClass();
	PlayerControllerClass = ASurviveThePlanetPlayerController::StaticClass();
	BuildToolbarWidgetClass = UBuildToolbarWidget::StaticClass();
	ResourceDisplayWidgetClass = UResourceDisplayWidget::StaticClass();
	EnergyModuleClass = AEnergyModule::StaticClass();
	MiningMachineClass = AMiningMachine::StaticClass();
}

void ASurviveThePlanetGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT GameMode active: %s, DefaultPawnClass=%s, PlayerControllerClass=%s"),
		*GetClass()->GetPathName(),
		*GetNameSafe(DefaultPawnClass),
		*GetNameSafe(PlayerControllerClass));

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASurviveThePlanetPlayerController* STPPlayerController = Cast<ASurviveThePlanetPlayerController>(PlayerController))
		{
			STPPlayerController->SetEnergyModuleClass(EnergyModuleClass);
			STPPlayerController->SetMiningMachineClass(MiningMachineClass);
		}

		if (BuildToolbarWidgetClass)
		{
			BuildToolbarWidget = CreateWidget<UBuildToolbarWidget>(PlayerController, BuildToolbarWidgetClass);
			if (BuildToolbarWidget)
			{
				BuildToolbarWidget->AddToViewport(10);
				UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Added build toolbar widget: %s"), *GetNameSafe(BuildToolbarWidget));
			}
		}
		else
		{
			UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD BuildToolbarWidgetClass is null; no build toolbar created."));
		}

		if (ResourceDisplayWidgetClass)
		{
			ResourceDisplayWidget = CreateWidget<UResourceDisplayWidget>(
				PlayerController, ResourceDisplayWidgetClass);
			if (ResourceDisplayWidget)
			{
				ResourceDisplayWidget->AddToViewport(10);
			}
		}
	}
}
