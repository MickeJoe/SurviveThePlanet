#include "Gameplay/UI/WorldHoverTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/ResourceCatalog.h"

FText UWorldHoverTooltipWidget::GetActorDisplayName(const AActor* Actor)
{
    if (!IsValid(Actor)) return FText::GetEmpty();
    if (const ABaseBuilding* Building = Cast<ABaseBuilding>(Actor))
        return Building->IsPlacementPreview() ? FText::GetEmpty() : Building->GetBuildingDisplayName();
    if (const ABaseDrone* Drone = Cast<ABaseDrone>(Actor)) return Drone->GetDroneDisplayName();
    if (const ABaseResourceSource* Source = Cast<ABaseResourceSource>(Actor))
    {
        for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
            if (Definition.ResourceType == Source->GetResourceType()) return Definition.DisplayName;
    }
    return FText::GetEmpty();
}

void UWorldHoverTooltipWidget::SetHoveredActor(const AActor* Actor)
{
    SetDisplayText(GetActorDisplayName(Actor));
}

void UWorldHoverTooltipWidget::SetDisplayText(const FText& Text, float WrapWidth)
{
    if (NameText)
    {
        NameText->SetText(Text);
        NameText->SetWrapTextAt(WrapWidth);
    }
    SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
