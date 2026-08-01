#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "EnergyStorageBuilding.generated.h"

/** Expands the HQ grid's maximum stored energy while completed and connected. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AEnergyStorageBuilding : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AEnergyStorageBuilding();

	void SetPlacementPreview(bool bPreview);
	void SetPlacementPreviewValid(bool bValidPlacement);

private:
	bool bPlacementPreview = false;
	bool bPlacementPreviewValid = true;
	void RefreshPlacementPreviewVisual();
};
