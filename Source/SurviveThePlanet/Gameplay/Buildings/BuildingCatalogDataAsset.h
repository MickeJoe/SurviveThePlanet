#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuildingCatalogDataAsset.generated.h"

class UBuildingDataAsset;

/** Ordered collection of every building definition available to a game mode. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UBuildingCatalogDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buildings")
	TArray<TObjectPtr<UBuildingDataAsset>> Buildings;
};
