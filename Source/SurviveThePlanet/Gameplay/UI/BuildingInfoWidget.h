#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "BuildingInfoWidget.generated.h"

class ABaseBuilding;
class UBorder;
class UHorizontalBox;
class UImage;
class UTextBlock;
class ACableNetworkManager;
class AResourceManager;
class UTexture2D;
class UBuildingInfoWidget;
class UUserWidget;

/** Data-aware button used inside WBP_DroneSelectionCard. */
UCLASS()
class SURVIVETHEPLANET_API UDroneTypeSelectionButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(UBuildingInfoWidget* InOwnerWidget, TSubclassOf<class ABaseDrone> InDroneClass, int32 InSlotIndex);

private:
	TWeakObjectPtr<UBuildingInfoWidget> OwnerWidget;
	TSubclassOf<class ABaseDrone> DroneClass;
	int32 SlotIndex = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();
};

/** Runtime-created button that remembers which building slot it represents. */
UCLASS()
class SURVIVETHEPLANET_API UDroneSlotButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(UBuildingInfoWidget* InOwnerWidget, int32 InSlotIndex);

private:
	TWeakObjectPtr<UBuildingInfoWidget> OwnerWidget;
	int32 SlotIndex = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();
};

/** Recipe buttons retain a stable choice while their WBP cards are refreshed. */
UCLASS()
class SURVIVETHEPLANET_API UWorkshopRecipeButton : public UButton
{
	GENERATED_BODY()
public:
	void Configure(UBuildingInfoWidget* InOwner, int32 InRecipeIndex);
private:
	TWeakObjectPtr<UBuildingInfoWidget> OwnerWidget;
	int32 RecipeIndex = 0;
	UFUNCTION() void HandleClicked();
};

/** UMG-backed building popup. Visual layout lives in WBP_BuildingInfoPopup. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UBuildingInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool CloseDroneSelectionIfOpen();
	void SelectWorkshopRecipe(int32 RecipeIndex);
	UFUNCTION(BlueprintCallable, Category = "Building Info")
	void SetBuilding(ABaseBuilding* NewBuilding);

	void HandleDroneSlotClicked(int32 SlotIndex);
	void AssignDroneOfClass(TSubclassOf<class ABaseDrone> DroneClass, int32 SlotIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UImage> BuildingImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UTextBlock> BuildingNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UTextBlock> BuildingDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UHorizontalBox> InputsBox;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UHorizontalBox> OutputsBox;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UTextBlock> CycleText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<class UProgressBar> CycleProgress;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UImage> PowerStatusImage;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UImage> ConsumptionIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UImage> GenerationIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UTextBlock> ConsumptionText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info") TObjectPtr<UTextBlock> GenerationText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info")
	TObjectPtr<UTextBlock> PowerStatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UBorder> DroneAssignmentSection;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UHorizontalBox> DroneSlotsBox;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UHorizontalBox> RecipeButtonsBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UWidget> RecipeSelectionSection;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<class UWidget> ProductionControls;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> StartProductionButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PauseProductionButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ClosePopupButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DroneCountText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CycleProgressText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SelectedRecipeText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Info|Drone Slots")
	TObjectPtr<UTexture2D> LockedDroneSlotIcon;

private:
	UPROPERTY(Transient)
	TObjectPtr<ABaseBuilding> Building;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DroneSelectionWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Info|Drone Selection", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> DroneSelectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Info|Drone Selection", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> DroneSelectionCardClass;

	UPROPERTY(Transient)
	TObjectPtr<ACableNetworkManager> CableNetworkManager;

	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	UFUNCTION() void HandleStartProduction();
	UFUNCTION() void HandlePauseProduction();
	UFUNCTION() void HandleClosePopup();
	void BuildRecipeSelection();
	void RefreshRecipeSelection();
	UPROPERTY(Transient) TArray<TObjectPtr<UUserWidget>> RecipeCards;
	void BuildProductionFlow();
	void RefreshProductionFlow();
	UUserWidget* AddResourceCard(UHorizontalBox* Box, EResourceType Resource, float Amount, bool bPerMinute);
	TArray<FResourceCost> DisplayedInputs;
	UPROPERTY(Transient) TArray<TObjectPtr<UUserWidget>> InputCards;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> LiveOutputCard;
	float DisplayedCycleSeconds = 0.0f;
	float FlowRefreshSeconds = 0.0f;
	void RefreshDroneSlots();
	void RefreshPowerStatus();
	void OpenDroneSelection(int32 SlotIndex);

	UFUNCTION()
	void CloseDroneSelection();
	void UnbindBuilding();

	UFUNCTION()
	void HandleDroneSlotsChanged(int32 UnlockedSlots);

	UFUNCTION()
	void HandleDroneAssignmentsChanged();

	UFUNCTION()
	void HandleCableNetworkChanged();

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);
};
