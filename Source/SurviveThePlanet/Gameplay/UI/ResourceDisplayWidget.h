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

private:
	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	void ResolveResourceManager();
	void RefreshAllResources();
	void ApplyConfiguredIcons();
	UImage* GetResourceImage(EResourceType ResourceType) const;
	UTextBlock* GetResourceAmountText(EResourceType ResourceType) const;

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);
};
