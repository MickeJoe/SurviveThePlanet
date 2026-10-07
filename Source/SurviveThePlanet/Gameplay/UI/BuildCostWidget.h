#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "BuildCostWidget.generated.h"
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;

/** Shared WBP presentation for placement totals and build-button tooltips. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UBuildCostWidget : public UUserWidget
{
	GENERATED_BODY()
	friend class UBuildToolbarWidget;
public:
	void Configure(ESTPBuildTool Tool, bool bPlacement, const FText& Tooltip = FText::GetEmpty());
protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UHorizontalBox> CostRow;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> BuildingIcon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(EditDefaultsOnly, Category="Resources") TMap<EResourceType, TObjectPtr<UTexture2D>> ResourceIcons;
private:
	ESTPBuildTool BuildTool = ESTPBuildTool::None;
	bool bPlacementView = false;
	TArray<FResourceCost> DisplayedCosts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> AmountTexts;
	void Refresh();
};
