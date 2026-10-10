#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VisitingMerchantWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UProgressBar;
class UBorder;

/** Bindings for the compact Cargo Bay HUD authored in WBP_VisitingMerchant. */
UCLASS()
class SURVIVETHEPLANET_API UVisitingMerchantWidget : public UUserWidget
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> TradeButtonLabel;
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UBorder> MerchantPanel;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> MerchantPortrait;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> MerchantNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> ArrivalText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UProgressBar> ArrivalProgress;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> TradeButton;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
	float RefreshElapsed = 0.0f;
	void Refresh();
	UFUNCTION()
	void OpenTrade();
};
