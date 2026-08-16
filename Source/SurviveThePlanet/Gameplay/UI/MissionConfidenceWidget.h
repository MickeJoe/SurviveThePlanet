#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionConfidenceWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UMissionConfidenceSubsystem;

/** Native behaviour for WBP_MissionConfidence. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UMissionConfidenceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Confidence")
	TObjectPtr<UHorizontalBox> ConfidenceSegments;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Confidence")
	TObjectPtr<UTextBlock> ConfidencePercentText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Mission Confidence")
	TObjectPtr<UTextBlock> ConfidenceDecayText;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMissionConfidenceSubsystem> ConfidenceSubsystem;

	UFUNCTION()
	void HandleConfidenceChanged(float NewConfidence, float Delta);

	void Refresh(float Confidence);
	void BuildSegments(float Confidence);
};
