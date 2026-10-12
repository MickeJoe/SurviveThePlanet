#pragma once
#include "CoreMinimal.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "UraniumResourceSource.generated.h"
class UIndustrialEffectsComponent;

/** Low exposed uranium-bearing rock; uses the shared finite-deposit mining system. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AUraniumResourceSource : public ABaseResourceSource
{
 GENERATED_BODY()
public:
 AUraniumResourceSource();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UIndustrialEffectsComponent> Effects;
};
