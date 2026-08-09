// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SurviveThePlanetGameMode.generated.h"

class UBuildToolbarWidget;
class UResourceDisplayWidget;
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

	/** Optional game-mode-specific catalog. The subsystem default is used when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buildings")
	TObjectPtr<UBuildingCatalogDataAsset> BuildingCatalog;

	UPROPERTY(Transient)
	TObjectPtr<UBuildToolbarWidget> BuildToolbarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UResourceDisplayWidget> ResourceDisplayWidget;
};



