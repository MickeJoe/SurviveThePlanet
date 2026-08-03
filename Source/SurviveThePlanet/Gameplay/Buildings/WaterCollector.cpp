#include "Gameplay/Buildings/WaterCollector.h"

#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Resources/ResourceManager.h"

AWaterCollector::AWaterCollector()
{
	BuildingTag = TEXT("WaterCollector");
	BuildingType = ESTPBuildingType::WaterCollector;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultWaterCollectorName", "Water Collector");
	EnergyStorageCapacity = 0.0f;
	EnergyProductionPerMinute = 0.0f;
	EnergyConsumptionPerMinute = 5.0f;
	ConstructionProgress = 0.0f;
	BuildingData = LoadObject<UWaterCollectorBuildingDataAsset>(nullptr, TEXT("/Game/Data/Buildings/DA_WaterCollector.DA_WaterCollector"));
}

void AWaterCollector::BeginPlay()
{
	Super::BeginPlay();
	ResolveWeatherManager();
}

void AWaterCollector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPlacementPreview || GetConstructionProgress() < 1.0f || !IsOperational())
	{
		return;
	}

	const float OutputPerMinute = GetCurrentWaterProductionPerMinute();
	if (OutputPerMinute <= 0.0f)
	{
		return;
	}

	PendingWaterOutput += OutputPerMinute * FMath::Max(0.0f, DeltaSeconds) / 60.0f;
	const int32 ProducedWater = FMath::FloorToInt(PendingWaterOutput);
	if (ProducedWater <= 0)
	{
		return;
	}

	PendingWaterOutput -= ProducedWater;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			It->AddResource(EResourceType::Water, ProducedWater);
			break;
		}
	}
}

float AWaterCollector::GetCurrentRainfallMmPerHour() const
{
	if (!IsValid(WeatherManager))
	{
		const_cast<AWaterCollector*>(this)->ResolveWeatherManager();
	}

	return IsValid(WeatherManager)
		? FMath::Max(0.0f, WeatherManager->GetCurrentWeather().PrecipitationPercent)
		: 0.0f;
}

float AWaterCollector::GetCurrentWaterProductionPerMinute() const
{
	return GetConstructionProgress() >= 1.0f
		? GetCurrentRainfallMmPerHour() * GetWaterPerMinutePerMmOfRain()
		: 0.0f;
}

float AWaterCollector::GetEnergyConsumptionPerMinute() const
{
	return !bPlacementPreview && GetCurrentWaterProductionPerMinute() > 0.0f
		? Super::GetEnergyConsumptionPerMinute()
		: 0.0f;
}

void AWaterCollector::SetPlacementPreview(bool bPreview)
{
	bPlacementPreview = bPreview;
	bIsSelectable = !bPreview;
	SetActorEnableCollision(!bPreview);
	SetConstructionProgress(bPreview ? 1.0f : 0.0f);
	if (BuildingMesh)
	{
		BuildingMesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		BuildingMesh->SetRenderCustomDepth(bPreview);
	}
	RefreshPlacementPreviewVisual();
}

void AWaterCollector::SetPlacementPreviewValid(bool bValidPlacement)
{
	bPlacementPreviewValid = bValidPlacement;
	RefreshPlacementPreviewVisual();
}

void AWaterCollector::RefreshPlacementPreviewVisual()
{
	if (BuildingMesh)
	{
		BuildingMesh->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}

void AWaterCollector::ResolveWeatherManager()
{
	WeatherManager = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetWeatherManager> It(World); It; ++It)
		{
			WeatherManager = *It;
			break;
		}
	}
}

const UWaterCollectorBuildingDataAsset* AWaterCollector::GetWaterCollectorBuildingData() const
{
	return Cast<UWaterCollectorBuildingDataAsset>(GetBuildingData());
}

float AWaterCollector::GetWaterPerMinutePerMmOfRain() const
{
	if (const UWaterCollectorBuildingDataAsset* WaterData = GetWaterCollectorBuildingData())
	{
		return FMath::Max(0.0f, WaterData->WaterPerMinutePerMmOfRain);
	}

	return FMath::Max(0.0f, WaterPerMinutePerMmOfRain);
}
