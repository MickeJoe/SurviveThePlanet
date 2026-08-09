#include "Gameplay/UI/ResourceDisplayWidget.h"

#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Buildings/WaterCollector.h"
#include "Gameplay/Buildings/ConcretePlant.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"

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
		{ EResourceType::Concrete, NSLOCTEXT("SurviveThePlanet", "ConcreteResourceTooltip", "Concrete"), nullptr }
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

	if (!bTimePaused)
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
		TimeScale = FMath::Clamp(Save->TimeScale, 1.0f, 3.0f);
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
	const int32 DayNumber = static_cast<int32>(WholeMinutes / 1440) + 1;
	const int32 MinuteOfDay = static_cast<int32>(WholeMinutes % 1440);
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
		PhaseText->SetText(FText::FromString(Phase));
	}
}

void UResourceDisplayWidget::SetGameTimeScale(float NewTimeScale)
{
	TimeScale = FMath::Clamp(NewTimeScale, 1.0f, 3.0f);
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

	SetSelected(PauseTimeButton, bTimePaused);
	SetSelected(Speed1Button, !bTimePaused && FMath::IsNearlyEqual(TimeScale, 1.0f));
	SetSelected(Speed15Button, !bTimePaused && FMath::IsNearlyEqual(TimeScale, 1.5f));
	SetSelected(Speed2Button, !bTimePaused && FMath::IsNearlyEqual(TimeScale, 2.0f));
	SetSelected(Speed3Button, !bTimePaused && FMath::IsNearlyEqual(TimeScale, 3.0f));
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
		bTimePaused ? PausedSimulationDilation : TimeScale);
}

void UResourceDisplayWidget::HandlePauseTimeClicked()
{
	bTimePaused = !bTimePaused;
	ApplySimulationRate();
	RefreshTimeControlStyles();
	SaveGameTime();
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
		EResourceType::Stone,
		EResourceType::Water,
		EResourceType::Concrete
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
	float WaterRatePerMinute = 0.0f;
	float ConcreteRatePerMinute = 0.0f;

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
	if (WaterRateText)
	{
		WaterRateText->SetText(FormatRate(WaterRatePerMinute));
	}
	if (ConcreteRateText)
	{
		ConcreteRateText->SetText(FormatRate(ConcreteRatePerMinute));
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
	case EResourceType::Water:
		return WaterIcon;
	case EResourceType::Concrete:
		return ConcreteIcon;
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
	case EResourceType::Water:
		return WaterAmountText;
	case EResourceType::Concrete:
		return ConcreteAmountText;
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
