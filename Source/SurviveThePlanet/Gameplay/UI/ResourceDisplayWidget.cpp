#include "Gameplay/UI/ResourceDisplayWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"

UResourceDisplayWidget::UResourceDisplayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Resources = {
		{ EResourceType::Energy, NSLOCTEXT("SurviveThePlanet", "EnergyResourceTooltip", "Energy"), nullptr },
		{ EResourceType::Iron, NSLOCTEXT("SurviveThePlanet", "IronResourceTooltip", "Iron"), nullptr },
		{ EResourceType::ControlChip, NSLOCTEXT("SurviveThePlanet", "ControlChipResourceTooltip", "Control Chip"), nullptr }
	};
}

void UResourceDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyConfiguredIcons();
}

void UResourceDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveResourceManager();
	RefreshAllResources();
}

void UResourceDisplayWidget::NativeDestruct()
{
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleResourceAmountChanged);
	}

	Super::NativeDestruct();
}
void UResourceDisplayWidget::ResolveResourceManager()
{
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleResourceAmountChanged);
	}

	ResourceManager = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			ResourceManager = *It;
			break;
		}
	}

	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.AddUniqueDynamic(
			this, &UResourceDisplayWidget::HandleResourceAmountChanged);
	}
}

void UResourceDisplayWidget::RefreshAllResources()
{
	for (const FResourceDisplayConfig& Config : Resources)
	{
		const int32 Amount = ResourceManager
			? ResourceManager->GetResourceAmount(Config.ResourceType)
			: 0;
		HandleResourceAmountChanged(Config.ResourceType, Amount);
	}
}

void UResourceDisplayWidget::ApplyConfiguredIcons()
{
	for (const FResourceDisplayConfig& Config : Resources)
	{
		if (UImage* Image = GetResourceImage(Config.ResourceType))
		{
			if (Config.IconTexture)
			{
				Image->SetBrushFromTexture(Config.IconTexture, true);
				Image->SetColorAndOpacity(FLinearColor::White);
			}
		}
	}
}

UImage* UResourceDisplayWidget::GetResourceImage(EResourceType ResourceType) const
{
	switch (ResourceType)
	{
	case EResourceType::Energy:
		return EnergyIcon;
	case EResourceType::Iron:
		return IronIcon;
	case EResourceType::ControlChip:
		return ControlChipIcon;
	default:
		return nullptr;
	}
}

UTextBlock* UResourceDisplayWidget::GetResourceAmountText(EResourceType ResourceType) const
{
	switch (ResourceType)
	{
	case EResourceType::Energy:
		return EnergyAmountText;
	case EResourceType::Iron:
		return IronAmountText;
	case EResourceType::ControlChip:
		return ControlChipAmountText;
	default:
		return nullptr;
	}
}

void UResourceDisplayWidget::HandleResourceAmountChanged(
	EResourceType ResourceType,
	int32 NewAmount)
{
	if (UTextBlock* AmountText = GetResourceAmountText(ResourceType))
	{
		AmountText->SetText(FText::AsNumber(NewAmount));
	}
}
