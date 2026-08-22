// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SurviveThePlanetGameMode.generated.h"

class UBuildToolbarWidget;
class UResourceDisplayWidget;
class UObjectiveTrackerWidget;
class UMissionConfidenceWidget;
class UMissionChoiceWidget;
class UTraderPanelWidget;
class AEnergyModule;
class AMiningMachine;
class UBuildingCatalogDataAsset;

/**
 *  Simple Game Mode for a top-down perspective game
 *  Sets the default gameplay framework classes
 *  Check the Blueprint derived class for the set values
 */
UCLASS()
class ASurviveThePlanetGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	ASurviveThePlanetGameMode();

	virtual void StartPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UBuildToolbarWidget> BuildToolbarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UResourceDisplayWidget> ResourceDisplayWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UObjectiveTrackerWidget> ObjectiveTrackerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMissionConfidenceWidget> MissionConfidenceWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UMissionChoiceWidget> MissionChoiceWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UTraderPanelWidget> TraderPanelWidgetClass;

	/** Optional game-mode-specific catalog. The subsystem default is used when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buildings")
	TObjectPtr<UBuildingCatalogDataAsset> BuildingCatalog;

	UPROPERTY(Transient)
	TObjectPtr<UBuildToolbarWidget> BuildToolbarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UResourceDisplayWidget> ResourceDisplayWidget;

	UPROPERTY(Transient)
	TObjectPtr<UObjectiveTrackerWidget> ObjectiveTrackerWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMissionConfidenceWidget> MissionConfidenceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UMissionChoiceWidget> MissionChoiceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTraderPanelWidget> TraderPanelWidget;
};



