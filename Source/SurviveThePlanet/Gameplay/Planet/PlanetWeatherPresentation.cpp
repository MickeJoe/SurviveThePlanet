#include "PlanetWeatherManager.h"
#include "PlanetDefinition.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Pawn.h"

float APlanetWeatherManager::WetWeatherProbability() const
{
	// Liquid rain is unavailable below freezing. Snow is a separate future effect.
	return PlanetDefinition && PlanetDefinition->MeanTemperatureCelsius > 0.0f
		? FMath::Clamp(PlanetDefinition->RainProbability, 0.0f, 1.0f) : 0.0f;
}
float APlanetWeatherManager::SolarElevation(double Minutes) const
{
	return PlanetDefinition ? PlanetDefinition->GetSolarElevation(Minutes) : 0.0f;
}
FText APlanetWeatherManager::GetDayPhase(double Minutes) const
{
	const float Elevation = SolarElevation(Minutes);
	const double DayMinutes = PlanetDefinition ? PlanetDefinition->GetDayLengthMinutes() : 1440.0;
	const bool Morning = FMath::Fmod(Minutes, DayMinutes) < DayMinutes*0.5;
	return FText::FromString(Elevation < -6 ? TEXT("Night") : Elevation < 10 ? (Morning ? TEXT("Dawn") : TEXT("Dusk")) : TEXT("Day"));
}
void APlanetWeatherManager::InitializePresentation()
{
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		SunLight = It->GetComponent(); break;
	}
	if (!SunLight) SunLight = GetWorld()->SpawnActor<ADirectionalLight>()->GetComponent();
	// Preserve the authored exposure/lighting calibration while changing relative irradiance.
	ClearSkySunIntensity = FMath::Max(SunLight->Intensity, 1.0f);
	SunLight->SetMobility(EComponentMobility::Movable);
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It) { SkyLight=It->GetLightComponent(); break; }
	if (!SkyLight) SkyLight = GetWorld()->SpawnActor<ASkyLight>()->GetLightComponent();
	ClearSkyAmbientIntensity = FMath::Max(SkyLight->Intensity, 0.1f);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCaptureEnabled(true);
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It) { Fog=It->GetComponent(); break; }
	if (!Fog) Fog=GetWorld()->SpawnActor<AExponentialHeightFog>()->GetComponent();
	WindSource = NewObject<UWindDirectionalSourceComponent>(this);
	WindSource->RegisterComponent();
	RainStreaks = NewObject<UInstancedStaticMeshComponent>(this);
	RainStreaks->SetMobility(EComponentMobility::Movable);
	RainStreaks->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	if (auto Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weather/M_RainStreak.M_RainStreak"))) RainStreaks->SetMaterial(0, Material);
	RainStreaks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RainStreaks->SetCastShadow(false);
	RainStreaks->RegisterComponent();
	WindMotes = NewObject<UInstancedStaticMeshComponent>(this);
	WindMotes->SetMobility(EComponentMobility::Movable);
	WindMotes->SetStaticMesh(RainStreaks->GetStaticMesh());
	WindMotes->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weather/M_WindMote.M_WindMote")));
	WindMotes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WindMotes->SetCastShadow(false);
	WindMotes->RegisterComponent();
	RainRandomStream.Initialize(PlanetDefinition ? PlanetDefinition->Seed + 71 : 71);
	for (int32 Index=0; Index<600; ++Index)
	{
		RainPositions.Add(FVector(RainRandomStream.FRandRange(-2200,2200), RainRandomStream.FRandRange(-2200,2200), RainRandomStream.FRandRange(100,1200)));
		RainStreaks->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector), true);
		if (Index < 120)
		{
			WindPositions.Add(FVector(RainRandomStream.FRandRange(-2200,2200), RainRandomStream.FRandRange(-2200,2200), RainRandomStream.FRandRange(50,450)));
			WindMotes->AddInstance(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector), true);
		}
	}
}
void APlanetWeatherManager::UpdateRain(float DeltaSeconds)
{
	if (!RainStreaks) return;
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Center = Pawn ? FVector(Pawn->GetActorLocation().X,Pawn->GetActorLocation().Y,GetActorLocation().Z) : GetActorLocation();
	const int32 VisibleCount=FMath::Clamp(FMath::RoundToInt(CurrentWeather.PrecipitationPercent * 45),0,600);
	const FVector Velocity(CurrentWeather.WindPercent * 100, CurrentWeather.WindPercent * 30, -1000);
	for (int32 Index=0; Index<RainPositions.Num(); ++Index)
	{
		FVector& P=RainPositions[Index]; P+=Velocity*DeltaSeconds;
		if(P.Z<100 || FMath::Abs(P.X)>2200 || FMath::Abs(P.Y)>2200) P=FVector(RainRandomStream.FRandRange(-2200,2200),RainRandomStream.FRandRange(-2200,2200),1200);
		const FVector Scale=Index<VisibleCount ? FVector(0.015f,0.015f,0.65f) : FVector::ZeroVector;
		const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector, -Velocity.GetSafeNormal());
		RainStreaks->UpdateInstanceTransform(Index,FTransform(Rotation,Center+P,Scale),true,false,true);
	}
	RainStreaks->MarkRenderStateDirty();
	if (!WindMotes) return;
	const int32 WindCount = FMath::Clamp(FMath::RoundToInt(CurrentWeather.WindPercent * 5), 0, 120);
	const FVector WindVelocity(Velocity.X, Velocity.Y, 0);
	for (int32 Index = 0; Index < WindPositions.Num(); ++Index)
	{
		FVector& P = WindPositions[Index]; P += WindVelocity * DeltaSeconds;
		if (P.X > 2200 || P.Y > 2200) P = FVector(-2200, RainRandomStream.FRandRange(-2200, 2200), RainRandomStream.FRandRange(50, 450));
		const FVector Scale = Index < WindCount ? FVector(0.4f, 0.025f, 0.025f) : FVector::ZeroVector;
		WindMotes->UpdateInstanceTransform(Index, FTransform(WindVelocity.Rotation(), Center + P, Scale), true, false, true);
	}
	WindMotes->MarkRenderStateDirty();
}
void APlanetWeatherManager::UpdatePresentation(double Minutes)
{
	const float PreviousSunlight = GetCurrentWeather().SunPercent;
	LastPresentationMinutes = Minutes;
	if (!FMath::IsNearlyEqual(PreviousSunlight, GetCurrentWeather().SunPercent))
	{
		BroadcastWeather();
	}
	if (!PlanetDefinition || !SunLight) return;
	const float Elevation=SolarElevation(Minutes);
	AHexSectorGrid* Grid = nullptr;
	for (TActorIterator<AHexSectorGrid> It(GetWorld()); It; ++It) { Grid = *It; break; }
	TSet<int32> LitSectors;
	TArray<ABaseBuilding*> Buildings;
	for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
	{
		ABaseBuilding* Building = *It;
		Buildings.Add(Building);
		const ESTPBuildingType Type = Building->GetBuildingType();
		if (Grid && !Building->IsPlacementPreview() && Building->GetConstructionProgress() >= 1.0f
			&& (Type == ESTPBuildingType::BaseModule || Type == ESTPBuildingType::RemoteBase))
		{
			const int32 SectorId = Grid->GetSectorAtWorldLocation(Building->GetActorLocation());
			if (SectorId != INDEX_NONE) LitSectors.Add(SectorId);
		}
	}
	const float Daylight = FMath::Clamp(Elevation / 20.0f, 0.0f, 1.0f);
	const float Sunshine = FMath::Clamp(CurrentWeather.SunPercent / 100.0f, 0.0f, 1.0f);
	// Use the same sunlight and ambient attenuation as the scene. Overcast weather
	// can require work lights even at noon; no clock-hour threshold is involved.
	const float DirectBrightness = Daylight * FMath::Lerp(0.08f, 1.0f, Sunshine);
	const float AmbientBrightness = Daylight * FMath::Lerp(0.35f, 1.0f, Sunshine);
	const float NaturalBrightness = DirectBrightness * 0.7f + AmbientBrightness * 0.3f;
	const float NightAmount = 1.0f - FMath::SmoothStep(0.08f, 0.7f, NaturalBrightness);
	for (ABaseBuilding* Building : Buildings)
	{
		const bool bLitSector = Grid && LitSectors.Contains(Grid->GetSectorAtWorldLocation(Building->GetActorLocation()));
		Building->UpdateNightLighting(NightAmount, bLitSector);
	}

	const float Yaw=FMath::Fmod(Minutes/PlanetDefinition->GetDayLengthMinutes()*360.0,360.0);
	SunLight->SetWorldRotation(FRotator(-Elevation,Yaw,0));
	SunLight->SetIntensity(ClearSkySunIntensity*Daylight*FMath::Lerp(0.08f,1.0f,Sunshine));
	SunLight->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(1,0.35f,0.12f), FLinearColor(1,0.97f,0.88f),Daylight));
	if (SkyLight) SkyLight->SetIntensity(ClearSkyAmbientIntensity*FMath::Lerp(0.12f,1.0f,Daylight)*FMath::Lerp(0.35f,1.0f,Sunshine));
	if (Fog)
	{
		Fog->SetFogDensity(0.005f + (1-Sunshine)*0.025f + CurrentWeather.PrecipitationPercent*0.001f);
		Fog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(FLinearColor(0.025f,0.035f,0.08f),FLinearColor(0.5f,0.6f,0.7f),Daylight));
	}
	if (WindSource) { WindSource->SetSpeed(CurrentWeather.WindPercent); WindSource->SetStrength(CurrentWeather.WindPercent/10.0f); }
}
