#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ExplorerDroneActivationWidget.generated.h"

class AExplorerDrone;
class UButton;
class UImage;
class UTextBlock;

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UExplorerDroneActivationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Selection changes must refresh even while this widget is collapsed (and cannot tick).
	void RefreshFromSelection();

protected:
	UFUNCTION() void HandleLaunchClicked();

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> LaunchButton;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> DroneTitleText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> InventoryText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UImage> DroneThumbnailImage;
	UPROPERTY(Transient, BlueprintReadOnly, Category="Explorer Drone") TObjectPtr<AExplorerDrone> SelectedExplorerDrone;
};
