#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConstructionProgressBarWidget.generated.h"

class SProgressBar;

UCLASS(Blueprintable, BlueprintType, DisplayName = "Construction Progress Bar Widget")
class SURVIVETHEPLANET_API UConstructionProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Construction")
	void SetProgress(float NewProgress);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	float Progress = 0.0f;
	TSharedPtr<SProgressBar> ProgressBar;
};
