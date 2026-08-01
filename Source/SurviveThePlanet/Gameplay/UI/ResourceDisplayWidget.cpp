#include "Gameplay/UI/ResourceDisplayWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Resources/BaseResourceSource.h"

namespace
{
	FText FormatRate(float RatePerMinute)
	{
		const float DisplayRate = FMath::IsNearlyZero(RatePerMinute, 0.05f) ? 0.0f : RatePerMinute;
		const FString Sign = DisplayRate >= 0.0f ? TEXT("+") : TEXT("");
		return FText::FromString(FString::Printf(TEXT("%s%.1f/min"), *Sign, DisplayRate));
	}
}

UResourceDisplayWidget::UResourceDisplayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Resources = {
		{ EResourceType::Energy, NSLOCTEXT("SurviveThePlanet", "EnergyResourceTooltip", "Energy"), nullptr },
		{ EResourceType::Iron, NSLOCTEXT("SurviveThePlanet", "IronResourceTooltip", "Iron"), nullptr },
		{ EResourceType::ControlChip, NSLOCTEXT("SurviveThePlanet", "ControlChipResourceTooltip", "Control Chip"), nullptr },
		{ EResourceType::Copper, NSLOCTEXT("SurviveThePlanet", "CopperResourceTooltip", "Copper"), nullptr },
		{ EResourceType::Stone, NSLOCTEXT("SurviveThePlanet", "StoneResourceTooltip", "Stone"), nullptr }
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
	RefreshResourceRates();
}

void UResourceDisplayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RateRefreshAccumulator += InDeltaTime;
	if (RateRefreshAccumulator >= 0.25f)
	{
		RateRefreshAccumulator = 0.0f;
		RefreshResourceRates();
	}
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
	static constexpr EResourceType DisplayedResourceTypes[] = {
		EResourceType::Energy,
		EResourceType::Iron,
		EResourceType::ControlChip,
		EResourceType::Copper,
		EResourceType::Stone
	};

	for (const EResourceType ResourceType : DisplayedResourceTypes)
	{
		const int32 Amount = ResourceManager
			? ResourceManager->GetResourceAmount(ResourceType)
			: 0;
		HandleResourceAmountChanged(ResourceType, Amount);
	}
}

void UResourceDisplayWidget::RefreshResourceRates()
{
	float EnergyRatePerMinute = 0.0f;
	float IronRatePerMinute = 0.0f;
	float CopperRatePerMinute = 0.0f;
	float StoneRatePerMinute = 0.0f;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			// Net grid change: energy modules produce; operational buildings consume.
			It->RefreshEnergyGrid();
			EnergyRatePerMinute = It->GetGridProductionPerMinute()
				- It->GetGridConsumptionPerMinute();
			break;
		}

		for (TActorIterator<AMiningMachine> It(World); It; ++It)
		{
			if (const ABaseResourceSource* Source = It->GetResourceSource(); IsValid(Source))
			{
				switch (Source->GetResourceType())
				{
				case EResourceType::Iron:
					IronRatePerMinute += It->GetCurrentOutputPerMinute();
					break;
				case EResourceType::Copper:
					CopperRatePerMinute += It->GetCurrentOutputPerMinute();
					break;
				case EResourceType::Stone:
					StoneRatePerMinute += It->GetCurrentOutputPerMinute();
					break;
				default:
					break;
				}
			}
		}
	}

	if (EnergyRateText)
	{
		EnergyRateText->SetText(FormatRate(EnergyRatePerMinute));
	}
	if (IronRateText)
	{
		IronRateText->SetText(FormatRate(IronRatePerMinute));
	}
	if (CopperRateText)
	{
		CopperRateText->SetText(FormatRate(CopperRatePerMinute));
	}
	if (StoneRateText)
	{
		StoneRateText->SetText(FormatRate(StoneRatePerMinute));
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
	case EResourceType::Copper:
		return CopperIcon;
	case EResourceType::Stone:
		return StoneIcon;
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
	case EResourceType::Copper:
		return CopperAmountText;
	case EResourceType::Stone:
		return StoneAmountText;
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
