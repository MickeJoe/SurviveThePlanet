#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

/** Visual layout is authored in WBP_PauseMenu. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ResumeButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> PlanetSelectorButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> QuitButton;
private:
	UFUNCTION() void Resume();
	UFUNCTION() void SelectPlanet();
	UFUNCTION() void Quit();
};
