#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "MissionChoiceWidget.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UMissionChoiceWidget;

UCLASS()
class SURVIVETHEPLANET_API UMissionChoiceButton : public UButton
{
	GENERATED_BODY()

public:
	void InitializeChoice(UMissionChoiceWidget* InOwner, int32 InSlotIndex, FName InObjectiveId);
	FName GetObjectiveId() const { return ObjectiveId; }
	int32 GetSlotIndex() const { return SlotIndex; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UMissionChoiceWidget> OwnerWidget;
	FName ObjectiveId;
	int32 SlotIndex = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();
};

/** WBP-backed selector for JSON-defined mission objective groups. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UMissionChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void HandleCandidateClicked(int32 SlotIndex, FName ObjectiveId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UButton> OpenMissionChoicesButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UCanvasPanel> ModalOverlay;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UHorizontalBox> ChoiceColumns;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UTextBlock> ChoiceStatusText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UButton> ConfirmChoicesButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Choices")
	TObjectPtr<UButton> CloseMissionChoicesButton;

private:
	UPROPERTY(Transient)
	TObjectPtr<UObjectiveSubsystem> ObjectiveSubsystem;
	FSTPMissionChoiceGroupDefinition CurrentGroup;
	int32 PickerSlotIndex = INDEX_NONE;

	UFUNCTION() void OpenPopup();
	UFUNCTION() void ClosePopup();
	UFUNCTION() void HandleGroupConfirmed(FName ChoiceGroupId);

	void BuildCurrentGroup();
	void BuildSlotPicker(int32 SlotIndex);
	void SetPopupVisible(bool bVisible);
};
