#include "Gameplay/UI/ResourceDisplayWidget.h"

#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Buildings/WaterCollector.h"
#include "Gameplay/Buildings/ConcretePlant.h"
#include "Gameplay/Buildings/Steelworks.h"
#include "Gameplay/Buildings/PolymerPlant.h"
#include "Gameplay/Buildings/ConnectorPlant.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Planet/PlanetDefinition.h"

namespace
{
	constexpr TCHAR GameTimeSaveSlot[] = TEXT("SurviveThePlanet_GameTime");
	constexpr float GameMinutesPerRealSecond = 1.0f;

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
		{ EResourceType::Stone, NSLOCTEXT("SurviveThePlanet", "StoneResourceTooltip", "Stone"), nullptr },
		{ EResourceType::Water, NSLOCTEXT("SurviveThePlanet", "WaterResourceTooltip", "Water"), nullptr },
		{ EResourceType::Concrete, NSLOCTEXT("SurviveThePlanet", "ConcreteResourceTooltip", "Concrete"), nullptr },
		{ EResourceType::Steel, NSLOCTEXT("SurviveThePlanet", "SteelResourceTooltip", "Steel"), nullptr },
		{ EResourceType::Coal, NSLOCTEXT("SurviveThePlanet", "CoalResourceTooltip", "Coal"), nullptr }
	};
}

void UResourceDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyConfiguredIcons();
	RefreshCategoryDisplay();
}

void UResourceDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResolveCategoryWidgets();
	RefreshCategoryDisplay();
	ResolveResourceManager();
	RefreshAllResources();
	RefreshResourceRates();
	ResolveTimeWidgets();
	ResolvePlanetWeatherManager();
	LoadGameTime();
	// Every new play session begins at Day 1, 08:00 and paused.
	TotalGameMinutes = 480.0;
	bTimePaused = true;
	ApplySimulationRate();
	RefreshGameTimeDisplay();
	RefreshTimeControlStyles();
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

	if (!IsSimulationPaused())
	{
		// UMG ticks in UI time, independently of the world's global time
		// dilation, so apply the selected simulation rate explicitly.
		TotalGameMinutes += InDeltaTime * GameMinutesPerRealSecond * TimeScale;
		RefreshGameTimeDisplay();
	}

	TimeSaveAccumulator += InDeltaTime;
	if (TimeSaveAccumulator >= 10.0f)
	{
		TimeSaveAccumulator = 0.0f;
		SaveGameTime();
	}
}

void UResourceDisplayWidget::NativeDestruct()
{
	SaveGameTime();

	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleResourceAmountChanged);
		ResourceManager->OnCreditsChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleCreditsChanged);
	}

	if (PlanetWeatherManager)
	{
		PlanetWeatherManager->OnWeatherChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandlePlanetWeatherChanged);
	}

	Super::NativeDestruct();
}

void UResourceDisplayWidget::ResolveTimeWidgets()
{
	UUserWidget* WeatherWidget = Cast<UUserWidget>(GetWidgetFromName(TEXT("WeatherTimeDisplay")));
	if (!WeatherWidget)
	{
		return;
	}

	ClockText = Cast<UTextBlock>(WeatherWidget->GetWidgetFromName(TEXT("ClockText")));
	PhaseText = Cast<UTextBlock>(WeatherWidget->GetWidgetFromName(TEXT("PhaseText")));
	WindText = Cast<UTextBlock>(WeatherWidget->GetWidgetFromName(TEXT("WindText")));
	RainText = Cast<UTextBlock>(WeatherWidget->GetWidgetFromName(TEXT("RainText")));
	CloudText = Cast<UTextBlock>(WeatherWidget->GetWidgetFromName(TEXT("CloudText")));
	PauseTimeButton = Cast<UButton>(WeatherWidget->GetWidgetFromName(TEXT("PauseTimeButton")));
	Speed1Button = Cast<UButton>(WeatherWidget->GetWidgetFromName(TEXT("Speed1Button")));
	Speed15Button = Cast<UButton>(WeatherWidget->GetWidgetFromName(TEXT("Speed15Button")));
	Speed2Button = Cast<UButton>(WeatherWidget->GetWidgetFromName(TEXT("Speed2Button")));
	Speed3Button = Cast<UButton>(WeatherWidget->GetWidgetFromName(TEXT("Speed3Button")));

	if (PauseTimeButton) PauseTimeButton->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandlePauseTimeClicked);
	if (Speed1Button) Speed1Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleSpeed1Clicked);
	if (Speed15Button) Speed15Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleSpeed15Clicked);
	if (Speed2Button) Speed2Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleSpeed2Clicked);
	if (Speed3Button) Speed3Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleSpeed3Clicked);
}

void UResourceDisplayWidget::ResolvePlanetWeatherManager()
{
	if (PlanetWeatherManager)
	{
		PlanetWeatherManager->OnWeatherChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandlePlanetWeatherChanged);
	}

	PlanetWeatherManager = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetWeatherManager> It(World); It; ++It)
		{
			PlanetWeatherManager = *It;
			break;
		}
	}

	if (PlanetWeatherManager)
	{
		PlanetWeatherManager->OnWeatherChanged.AddUniqueDynamic(
			this, &UResourceDisplayWidget::HandlePlanetWeatherChanged);
		HandlePlanetWeatherChanged(PlanetWeatherManager->GetCurrentWeather());
	}
}

void UResourceDisplayWidget::HandlePlanetWeatherChanged(FPlanetWeatherState NewWeather)
{
	if (WindText)
	{
		WindText->SetText(FText::FromString(FString::Printf(
			TEXT("%.1f m/s"), NewWeather.WindPercent)));
	}

	if (RainText)
	{
		RainText->SetText(FText::FromString(FString::Printf(
			TEXT("%d mm/h"), FMath::RoundToInt(NewWeather.PrecipitationPercent))));
	}

	if (CloudText)
	{
		CloudText->SetText(FText::FromString(FString::Printf(
			TEXT("%d%%"), FMath::RoundToInt(NewWeather.SunPercent))));
	}
}

void UResourceDisplayWidget::LoadGameTime()
{
	if (UGameTimeSaveGame* Save = Cast<UGameTimeSaveGame>(UGameplayStatics::LoadGameFromSlot(GameTimeSaveSlot, 0)))
	{
		TotalGameMinutes = FMath::Max(0.0, Save->TotalGameMinutes);
		TimeScale = FMath::Clamp(Save->TimeScale, 1.0f, 30.0f);
		bTimePaused = Save->bTimePaused;
	}
}

void UResourceDisplayWidget::SaveGameTime() const
{
	UGameTimeSaveGame* Save = Cast<UGameTimeSaveGame>(UGameplayStatics::CreateSaveGameObject(UGameTimeSaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	Save->TotalGameMinutes = TotalGameMinutes;
	Save->TimeScale = TimeScale;
	Save->bTimePaused = bTimePaused;
	UGameplayStatics::SaveGameToSlot(Save, GameTimeSaveSlot, 0);
}

void UResourceDisplayWidget::RefreshGameTimeDisplay()
{
	const int64 WholeMinutes = FMath::Max<int64>(0, FMath::FloorToInt64(TotalGameMinutes));
	const UPlanetDefinition* Planet = PlanetWeatherManager ? PlanetWeatherManager->GetPlanetDefinition() : nullptr;
	const int64 DayMinutes = Planet ? static_cast<int64>(Planet->GetDayLengthMinutes()) : 1440;
	const int32 DayNumber = static_cast<int32>(WholeMinutes / DayMinutes) + 1;
	const int32 MinuteOfDay = static_cast<int32>(WholeMinutes % DayMinutes);
	if (PlanetWeatherManager) PlanetWeatherManager->UpdatePresentation(TotalGameMinutes);
	const int32 Hour = MinuteOfDay / 60;
	const int32 Minute = MinuteOfDay % 60;

	if (ClockText)
	{
		ClockText->SetText(FText::FromString(FString::Printf(TEXT("Day %d   %02d:%02d"), DayNumber, Hour, Minute)));
	}

	if (PhaseText)
	{
		const TCHAR* Phase = Hour >= 5 && Hour < 8 ? TEXT("Dawn")
			: Hour >= 8 && Hour < 18 ? TEXT("Day")
			: Hour >= 18 && Hour < 21 ? TEXT("Dusk")
			: TEXT("Night");
		PhaseText->SetText(PlanetWeatherManager ? PlanetWeatherManager->GetDayPhase(TotalGameMinutes) : FText::FromString(Phase));
	}
}

void UResourceDisplayWidget::SetGameTimeScale(float NewTimeScale)
{
#if !UE_BUILD_SHIPPING
	// Unreal's default global dilation ceiling is 20; permit the x30 cheat.
	GetWorld()->GetWorldSettings()->MaxGlobalTimeDilation = 30.0f;
#endif
	TimeScale = FMath::Clamp(NewTimeScale, 1.0f, 30.0f);
	bTimePaused = false;
	ApplySimulationRate();
	RefreshTimeControlStyles();
	SaveGameTime();
}

void UResourceDisplayWidget::RefreshTimeControlStyles()
{
	const FLinearColor SelectedColor(0.15f, 0.48f, 0.65f, 1.0f);
	const FLinearColor DefaultColor(0.06f, 0.09f, 0.10f, 1.0f);

	const auto SetSelected = [&SelectedColor, &DefaultColor](UButton* Button, bool bSelected)
	{
		if (Button)
		{
			Button->SetBackgroundColor(bSelected ? SelectedColor : DefaultColor);
		}
	};

	SetSelected(PauseTimeButton, IsSimulationPaused());
	SetSelected(Speed1Button, !IsSimulationPaused() && FMath::IsNearlyEqual(TimeScale, 1.0f));
	SetSelected(Speed15Button, !IsSimulationPaused() && FMath::IsNearlyEqual(TimeScale, 1.5f));
	SetSelected(Speed2Button, !IsSimulationPaused() && FMath::IsNearlyEqual(TimeScale, 2.0f));
	SetSelected(Speed3Button, !IsSimulationPaused() && FMath::IsNearlyEqual(TimeScale, 3.0f));
}

void UResourceDisplayWidget::ApplySimulationRate() const
{
	// A tiny non-zero dilation keeps Slate/input responsive, allowing the
	// player to select units and queue orders while the simulation is frozen.
	// At this value actors, timers, production and consumption are effectively
	// stopped, while choosing a speed restores/scales the entire world.
	constexpr float PausedSimulationDilation = 0.0001f;
	UGameplayStatics::SetGlobalTimeDilation(
		this,
		IsSimulationPaused() ? PausedSimulationDilation : TimeScale);
}

void UResourceDisplayWidget::HandlePauseTimeClicked()
{
	bTimePaused = !bTimePaused;
	ApplySimulationRate();
	RefreshTimeControlStyles();
	SaveGameTime();
}

void UResourceDisplayWidget::SetCheatSpeed10()
{
#if !UE_BUILD_SHIPPING
	SetGameTimeScale(10.0f);
#endif
}

void UResourceDisplayWidget::SetCheatSpeed30()
{
#if !UE_BUILD_SHIPPING
	SetGameTimeScale(30.0f);
#endif
}

void UResourceDisplayWidget::SetTradingPaused(bool bPaused)
{
	bTradingPaused = bPaused;
	ApplySimulationRate();
	RefreshTimeControlStyles();
}

void UResourceDisplayWidget::HandleSpeed1Clicked() { SetGameTimeScale(1.0f); }
void UResourceDisplayWidget::HandleSpeed15Clicked() { SetGameTimeScale(1.5f); }
void UResourceDisplayWidget::HandleSpeed2Clicked() { SetGameTimeScale(2.0f); }
void UResourceDisplayWidget::HandleSpeed3Clicked() { SetGameTimeScale(3.0f); }
void UResourceDisplayWidget::ResolveResourceManager()
{
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleResourceAmountChanged);
		ResourceManager->OnCreditsChanged.RemoveDynamic(
			this, &UResourceDisplayWidget::HandleCreditsChanged);
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
		ResourceManager->OnCreditsChanged.AddUniqueDynamic(
			this, &UResourceDisplayWidget::HandleCreditsChanged);
	}
}

void UResourceDisplayWidget::HandleCreditsChanged(int32 NewCredits)
{
	if (CreditsAmountText)
	{
		CreditsAmountText->SetText(FText::AsNumber(NewCredits));
	}
}

void UResourceDisplayWidget::RefreshAllResources()
{
	HandleCreditsChanged(ResourceManager ? ResourceManager->GetCredits() : 0);
	for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
	{
		HandleResourceAmountChanged(Definition.ResourceType,
			ResourceManager ? ResourceManager->GetResourceAmount(Definition.ResourceType) : 0);
	}
}

void UResourceDisplayWidget::RefreshResourceRates()
{
	float EnergyRatePerMinute = 0.0f;
	float IronRatePerMinute = 0.0f;
	float CopperRatePerMinute = 0.0f;
	float StoneRatePerMinute = 0.0f;
	float CoalRatePerMinute = 0.0f;
	float WaterRatePerMinute = 0.0f;
	float ConcreteRatePerMinute = 0.0f;
	float SteelRatePerMinute = 0.0f;
	float PolymerRatePerMinute = 0.0f;
	float ConnectorRatePerMinute = 0.0f;

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
				case EResourceType::Coal:
					CoalRatePerMinute += It->GetCurrentOutputPerMinute();
					break;
				case EResourceType::Stone:
					StoneRatePerMinute += It->GetCurrentOutputPerMinute();
					break;
				default:
					break;
				}
			}
		}

		for (TActorIterator<AWaterCollector> It(World); It; ++It)
		{
			if (It->IsOperational())
			{
				WaterRatePerMinute += It->GetCurrentWaterProductionPerMinute();
			}
		}

		for (TActorIterator<AConcretePlant> It(World); It; ++It)
		{
			if (It->IsOperational() && It->GetConstructionProgress() >= 1.0f)
			{
				ConcreteRatePerMinute += It->GetConcreteProductionPerMinute();
			}
		}

		for (TActorIterator<AConnectorPlant> It(World); It; ++It)
		{
			if (It->IsProducing())
			{
				ConnectorRatePerMinute += It->GetConnectorProductionPerMinute();
				CopperRatePerMinute -= It->GetCopperConsumptionPerMinute();
				PolymerRatePerMinute -= It->GetPolymerConsumptionPerMinute();
			}
		}
		for (TActorIterator<APolymerPlant> It(World); It; ++It)
		{
			if (It->IsProducing())
			{
				PolymerRatePerMinute += It->GetPolymerProductionPerMinute();
				CoalRatePerMinute -= It->GetCoalConsumptionPerMinute();
				WaterRatePerMinute -= It->GetWaterConsumptionPerMinute();
			}
		}
		for (TActorIterator<ASteelworks> It(World); It; ++It)
		{
			if (It->IsProducing())
			{
				SteelRatePerMinute += It->GetSteelProductionPerMinute();
				IronRatePerMinute -= It->GetIronConsumptionPerMinute();
			}
		}
	}

	const TMap<EResourceType, float> Rates = {
        {EResourceType::Energy, EnergyRatePerMinute},
        {EResourceType::Iron, IronRatePerMinute},
        {EResourceType::Copper, CopperRatePerMinute},
        {EResourceType::Coal, CoalRatePerMinute},
        {EResourceType::Stone, StoneRatePerMinute},
        {EResourceType::Water, WaterRatePerMinute},
        {EResourceType::Concrete, ConcreteRatePerMinute},
        {EResourceType::Polymer, PolymerRatePerMinute},
        {EResourceType::Connector, ConnectorRatePerMinute},
        {EResourceType::Steel, SteelRatePerMinute}
    };
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        if (UTextBlock* RateText = GetResourceRateText(Definition.ResourceType))
        {
            const float Rate = Rates.FindRef(Definition.ResourceType);
            RateText->SetText(FormatRate(Rate));
            RateText->SetColorAndOpacity(Rate < -0.05f
                ? FLinearColor(1.0f, 0.3f, 0.18f) : FLinearColor(0.68f, 0.9f, 0.28f));
        }
    }
}

void UResourceDisplayWidget::ApplyConfiguredIcons()
{
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        if (UImage* Image = GetResourceImage(Definition.ResourceType))
        {
            if (UTexture2D* Icon = Definition.Icon.LoadSynchronous())
            {
                Image->SetBrushFromTexture(Icon, false);
                Image->SetColorAndOpacity(FLinearColor::White);
            }
        }
    }
	for (const FResourceDisplayConfig& Config : Resources)
	{
		if (UImage* Image = GetResourceImage(Config.ResourceType))
		{
			if (Config.IconTexture)
			{
				Image->SetBrushFromTexture(Config.IconTexture, false);
				Image->SetColorAndOpacity(FLinearColor::White);
			}
		}
	}
}

UImage* UResourceDisplayWidget::GetResourceImage(EResourceType ResourceType) const
{
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        if (Definition.ResourceType == ResourceType)
        {
            return Cast<UImage>(GetWidgetFromName(FName(Definition.WidgetPrefix.ToString() + TEXT("Icon"))));
        }
    }
    return nullptr;
}

UTextBlock* UResourceDisplayWidget::GetResourceAmountText(EResourceType ResourceType) const
{
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        if (Definition.ResourceType == ResourceType)
        {
            return Cast<UTextBlock>(GetWidgetFromName(FName(Definition.WidgetPrefix.ToString() + TEXT("AmountText"))));
        }
    }
    return nullptr;
}

UTextBlock* UResourceDisplayWidget::GetResourceRateText(EResourceType ResourceType) const
{
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        if (Definition.ResourceType == ResourceType)
        {
            return Cast<UTextBlock>(GetWidgetFromName(FName(Definition.WidgetPrefix.ToString() + TEXT("RateText"))));
        }
    }
    return nullptr;
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

void UResourceDisplayWidget::ResolveCategoryWidgets()
{
    if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("RawMaterialsButton"))))
        Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleRawMaterialsClicked);
    if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("MaterialsButton"))))
        Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleMaterialsClicked);
    if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("ComponentsButton"))))
        Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleComponentsClicked);
    if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("AdvancedGoodsButton"))))
        Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleAdvancedGoodsClicked);
    if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("ToggleResourceCardsButton"))))
        Button->OnClicked.AddUniqueDynamic(this, &UResourceDisplayWidget::HandleToggleResourceCardsClicked);
}

void UResourceDisplayWidget::SelectResourceCategory(EResourceCategory Category)
{
    if (Category == EResourceCategory::Energy) return;
    SelectedResourceCategory = Category;
    bResourceCardsVisible = true;
    RefreshCategoryDisplay();
}

void UResourceDisplayWidget::SetResourceCardsVisible(bool bVisible)
{
    bResourceCardsVisible = bVisible;
    RefreshCategoryDisplay();
}

void UResourceDisplayWidget::RefreshCategoryDisplay()
{
    UUniformGridPanel* Grid = Cast<UUniformGridPanel>(GetWidgetFromName(TEXT("ResourceCardsGrid")));
    if (!Grid) return;
    Grid->SetVisibility(bResourceCardsVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    if (UWidget* Body = GetWidgetFromName(TEXT("ResourceCardsBody")))
        Body->SetVisibility(bResourceCardsVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    int32 ResourceCount = 0;
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
        if (Definition.Category == SelectedResourceCategory) ++ResourceCount;
    const int32 Columns = FMath::Max(1, (ResourceCount + 1) / 2);
    int32 Index = 0;
    for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
    {
        UWidget* Card = GetWidgetFromName(FName(Definition.WidgetPrefix.ToString() + TEXT("ResourceCard")));
        if (!Card) continue;
        const bool bEnergy = Definition.ResourceType == EResourceType::Energy;
        const bool bSelected = bEnergy || Definition.Category == SelectedResourceCategory;
        Card->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bSelected)
        {
            if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(Card->Slot))
            {
                GridSlot->SetRow(Index / Columns);
                GridSlot->SetColumn(Index % Columns);
            }
            if (!bEnergy) ++Index;
        }
    }
    static const TCHAR* Prefixes[] = {TEXT("RawMaterials"), TEXT("Materials"), TEXT("Components"), TEXT("AdvancedGoods")};
    for (int32 CategoryIndex = 0; CategoryIndex < 4; ++CategoryIndex)
    {
        const bool bSelected = static_cast<int32>(SelectedResourceCategory) == CategoryIndex;
        if (UButton* Button = Cast<UButton>(GetWidgetFromName(FName(FString(Prefixes[CategoryIndex]) + TEXT("Button")))))
            Button->SetBackgroundColor(bSelected ? FLinearColor(0.06f, 0.28f, 0.34f) : FLinearColor(0.04f, 0.07f, 0.09f));
        if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(FName(FString(Prefixes[CategoryIndex]) + TEXT("Label")))))
            Label->SetColorAndOpacity(bSelected ? FLinearColor(0.0f, 0.85f, 1.0f) : FLinearColor(0.75f, 0.8f, 0.82f));
    }
    if (UWidget* UpIcon = GetWidgetFromName(TEXT("ResourceCollapseUpIcon")))
        UpIcon->SetVisibility(bResourceCardsVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (UWidget* DownIcon = GetWidgetFromName(TEXT("ResourceCollapseDownIcon")))
        DownIcon->SetVisibility(bResourceCardsVisible ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    if (UButton* Toggle = Cast<UButton>(GetWidgetFromName(TEXT("ToggleResourceCardsButton"))))
        Toggle->SetToolTipText(FText::FromString(bResourceCardsVisible ? TEXT("Hide resources") : TEXT("Show resources")));
}

void UResourceDisplayWidget::HandleRawMaterialsClicked() { SelectResourceCategory(EResourceCategory::RawMaterials); }
void UResourceDisplayWidget::HandleMaterialsClicked() { SelectResourceCategory(EResourceCategory::Materials); }
void UResourceDisplayWidget::HandleComponentsClicked() { SelectResourceCategory(EResourceCategory::Components); }
void UResourceDisplayWidget::HandleAdvancedGoodsClicked() { SelectResourceCategory(EResourceCategory::AdvancedGoods); }
void UResourceDisplayWidget::HandleToggleResourceCardsClicked() { SetResourceCardsVisible(!bResourceCardsVisible); }
