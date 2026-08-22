#include "Gameplay/Cheats/STPCheatManager.h"

#include "EngineUtils.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "GameFramework/PlayerController.h"

bool USTPCheatManager::GiveResource(EResourceType ResourceType, int32 Amount)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (Amount <= 0 || !GetOuterAPlayerController())
	{
		return false;
	}

	UWorld* World = GetOuterAPlayerController()->GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AResourceManager> It(World); It; ++It)
	{
		It->AddResource(ResourceType, Amount);
		return true;
	}

	return false;
#endif
}
