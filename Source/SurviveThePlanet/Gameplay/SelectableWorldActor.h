#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SelectableWorldActor.generated.h"

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ASelectableWorldActor : public AActor
{
	GENERATED_BODY()

public:
	ASelectableWorldActor();

	UFUNCTION(BlueprintPure, Category = "Selection")
	bool IsWorldSelectable() const { return bIsSelectable; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection")
	bool bIsSelectable = true;
};
