#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldHoverTooltipWidget.generated.h"

class UTextBlock;

/** Shared in-game tooltip authored by WBP_WorldHoverTooltip. */
UCLASS()
class SURVIVETHEPLANET_API UWorldHoverTooltipWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Tooltips")
    static FText GetActorDisplayName(const AActor* Actor);
    UFUNCTION(BlueprintCallable, Category="Tooltips")
    void SetHoveredActor(const AActor* Actor);
    void SetDisplayText(const FText& Text, float WrapWidth = 0.0f);
protected:
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
};
