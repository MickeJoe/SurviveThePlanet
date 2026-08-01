#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "ResourceDisplayWidget.generated.h"

class AResourceManager;
class UImage;
class UTextBlock;
class UTexture2D;

USTRUCT(BlueprintType)
struct FResourceDisplayConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	EResourceType ResourceType = EResourceType::Energy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FText Tooltip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<UTexture2D> IconTexture;
};

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UResourceDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UResourceDisplayWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Configure labels and icon textures on a Widget Blueprint derived from this class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource Display")
	TArray<FResourceDisplayConfig> Resources;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> EnergyIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> EnergyAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> IronIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> IronAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> ControlChipIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> ControlChipAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> CopperIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> CopperAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> StoneIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> StoneAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> EnergyRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> IronRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> CopperRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> StoneRateText;

private:
	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	float RateRefreshAccumulator = 0.0f;

	void ResolveResourceManager();
	void RefreshAllResources();
	void RefreshResourceRates();
	void ApplyConfiguredIcons();
	UImage* GetResourceImage(EResourceType ResourceType) const;
	UTextBlock* GetResourceAmountText(EResourceType ResourceType) const;

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);
};
