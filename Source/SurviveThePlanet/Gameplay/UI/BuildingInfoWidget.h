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

/** UMG-backed building popup. Visual layout lives in WBP_BuildingInfoPopup. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UBuildingInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Building Info")
	void SetBuilding(ABaseBuilding* NewBuilding);

	void HandleDroneSlotClicked(int32 SlotIndex);
	void AssignDroneOfClass(TSubclassOf<class ABaseDrone> DroneClass, int32 SlotIndex);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UImage> BuildingImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UTextBlock> BuildingNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UTextBlock> BuildingDescriptionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Building Info")
	TObjectPtr<UTextBlock> PowerStatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UBorder> DroneAssignmentSection;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Building Info")
	TObjectPtr<UHorizontalBox> DroneSlotsBox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Building Info|Drone Slots")
	TObjectPtr<UTexture2D> LockedDroneSlotIcon;

private:
	UPROPERTY(Transient)
	TObjectPtr<ABaseBuilding> Building;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> DroneSelectionWidget;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> DroneSelectionWidgetClass;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> DroneSelectionCardClass;

	UPROPERTY(Transient)
	TObjectPtr<ACableNetworkManager> CableNetworkManager;

	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

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
