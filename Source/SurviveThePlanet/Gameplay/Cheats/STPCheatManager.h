#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "STPCheatManager.generated.h"

UCLASS(NotBlueprintable)
class SURVIVETHEPLANET_API USTPCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** Adds Amount of ResourceType to the active resource manager. Development builds only. */
	UFUNCTION(BlueprintCallable, Exec, Category = "Cheats|Resources", meta = (DevelopmentOnly))
	bool GiveResource(EResourceType ResourceType, int32 Amount);
};
