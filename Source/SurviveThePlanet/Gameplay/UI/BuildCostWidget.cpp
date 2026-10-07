#include "Gameplay/UI/BuildCostWidget.h"
#include "Gameplay/Resources/ResourceCatalog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "SurviveThePlanetPlayerController.h"
#include "EngineUtils.h"

void UBuildCostWidget::Configure(ESTPBuildTool Tool, bool bPlacement, const FText& Tooltip)
{
	if (BuildingIcon) BuildingIcon->SetDesiredSizeOverride(FVector2D(38.0f, 38.0f));
	BuildTool = Tool;
	bPlacementView = bPlacement;
	if (DescriptionText)
	{
		DescriptionText->SetText(Tooltip);
		DescriptionText->SetVisibility(Tooltip.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (bPlacementView) SetVisibility(Tool == ESTPBuildTool::None ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	else Refresh();
}

void UBuildCostWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	Refresh();
}

void UBuildCostWidget::Refresh()
{
	ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>();
	if (!Controller || !CostRow || !BuildingIcon || !StatusText) return;
	const ESTPBuildTool Tool = bPlacementView ? Controller->GetActiveBuildTool() : BuildTool;
	SetVisibility(Tool == ESTPBuildTool::None ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (Tool == ESTPBuildTool::None) return;

	const TArray<FResourceCost> Costs = Controller->GetBuildCosts(Tool, bPlacementView);
	bool bResourcesChanged = Costs.Num() != DisplayedCosts.Num();
	for (int32 Index = 0; !bResourcesChanged && Index < Costs.Num(); ++Index)
		bResourcesChanged = Costs[Index].Resource != DisplayedCosts[Index].Resource;
	if (bResourcesChanged)
	{
		CostRow->ClearChildren();
		AmountTexts.Reset();
		for (const FResourceCost& Cost : Costs)
		{
			UImage* Icon = WidgetTree->ConstructWidget<UImage>();
			UTexture2D* Texture = ResourceIcons.FindRef(Cost.Resource);
            if (!Texture) Texture = UResourceCatalog::GetResourceIcon(Cost.Resource);
            Icon->SetBrushFromTexture(Texture);
			Icon->SetDesiredSizeOverride(FVector2D(32,32));
			CostRow->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
			UTextBlock* Amount = WidgetTree->ConstructWidget<UTextBlock>();
			Amount->SetMinDesiredWidth(48.0f);
			Amount->SetJustification(ETextJustify::Right);
			FSlateFontInfo Font = Amount->GetFont(); Font.Size = 20; Font.TypefaceFontName = TEXT("Bold"); Amount->SetFont(Font);
			UHorizontalBoxSlot* AmountSlot = CostRow->AddChildToHorizontalBox(Amount);
			AmountSlot->SetPadding(FMargin(6,0,16,0)); AmountSlot->SetVerticalAlignment(VAlign_Center);
			AmountTexts.Add(Amount);
			if (!ResourceIcons.FindRef(Cost.Resource)) Icon->SetToolTipText(StaticEnum<EResourceType>()->GetDisplayNameTextByValue(static_cast<int64>(Cost.Resource)));
		}
	}
	AResourceManager* Resources = nullptr;
	for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It) { Resources = *It; break; }
	const bool bAffordable = Costs.IsEmpty() || (Resources && Resources->CanAffordCosts(Costs));
	for (int32 Index = 0; Index < AmountTexts.Num(); ++Index)
	{
		if (bResourcesChanged || Costs[Index].Cost != DisplayedCosts[Index].Cost)
		{
			AmountTexts[Index]->SetText(FText::AsNumber(Costs[Index].Cost));
		}
		const bool bEnough = Resources && Resources->GetResourceAmount(Costs[Index].Resource) >= Costs[Index].Cost;
		AmountTexts[Index]->SetColorAndOpacity(bEnough ? FLinearColor::White : FLinearColor(1,0.22f,0.16f,1));
	}
	DisplayedCosts = Costs;
	const ABaseBuilding* Preview = bPlacementView ? Controller->GetActivePlacementPreview() : nullptr;
	const bool bValid = bAffordable && (!bPlacementView || (Preview && !Preview->IsHidden() && Preview->IsPlacementPreviewValid()));
	StatusText->SetText(FText::FromString(bValid ? TEXT("\u2713") : TEXT("\u00D7")));
	StatusText->SetColorAndOpacity(bValid ? FLinearColor(0.25f,1,0.15f,1) : FLinearColor(1,0.22f,0.16f,1));
}
