// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurviveThePlanetGameMode.h"

#include "SurviveThePlanetCameraPawn.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/BuildingCatalogDataAsset.h"
#include "Gameplay/UI/BuildToolbarWidget.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "Gameplay/UI/ObjectiveTrackerWidget.h"
#include "Gameplay/UI/MissionConfidenceWidget.h"
#include "SurviveThePlanet.h"

ASurviveThePlanetGameMode::ASurviveThePlanetGameMode()
{
	DefaultPawnClass = ASurviveThePlanetCameraPawn::StaticClass();
	PlayerControllerClass = ASurviveThePlanetPlayerController::StaticClass();
	BuildToolbarWidgetClass = UBuildToolbarWidget::StaticClass();
	ResourceDisplayWidgetClass = UResourceDisplayWidget::StaticClass();
	ObjectiveTrackerWidgetClass = LoadClass<UObjectiveTrackerWidget>(nullptr,
		TEXT("/Game/UI/Objectives/WBP_ObjectiveTracker.WBP_ObjectiveTracker_C"));
	MissionConfidenceWidgetClass = LoadClass<UMissionConfidenceWidget>(nullptr,
		TEXT("/Game/UI/Mission/WBP_MissionConfidencePanel.WBP_MissionConfidencePanel_C"));
	BuildingCatalog = LoadObject<UBuildingCatalogDataAsset>(nullptr, TEXT("/Game/Data/Buildings/DA_BuildingCatalog.DA_BuildingCatalog"));
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
		if (BuildingCatalog)
		{
			if (UBuildingManagerSubsystem* Manager = GetWorld()->GetSubsystem<UBuildingManagerSubsystem>())
			{
				Manager->SetCatalog(BuildingCatalog);
			}
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

		if (ObjectiveTrackerWidgetClass)
		{
			ObjectiveTrackerWidget = CreateWidget<UObjectiveTrackerWidget>(PlayerController, ObjectiveTrackerWidgetClass);
			if (ObjectiveTrackerWidget)
			{
				ObjectiveTrackerWidget->AddToViewport(10);
			}
		}

		if (MissionConfidenceWidgetClass)
		{
			MissionConfidenceWidget = CreateWidget<UMissionConfidenceWidget>(PlayerController, MissionConfidenceWidgetClass);
			if (MissionConfidenceWidget)
			{
				MissionConfidenceWidget->AddToViewport(10);
			}
		}
	}
}
