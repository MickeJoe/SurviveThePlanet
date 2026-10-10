#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlanetSelectionGameMode.generated.h"
UCLASS()
class SURVIVETHEPLANET_API APlanetSelectionGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	APlanetSelectionGameMode();
	virtual void StartPlay() override;
};
