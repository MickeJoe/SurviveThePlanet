#include "Gameplay/Planet/MissionConfidenceSubsystem.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Planet/PlanetDefinition.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"

void UMissionConfidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResolvePlanetSettings();
}

bool UMissionConfidenceSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UMissionConfidenceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMissionConfidenceSubsystem, STATGROUP_Tickables);
}

void UMissionConfidenceSubsystem::Tick(float DeltaTime)
{
	if (!bSettingsResolved)
	{
		ResolvePlanetSettings();
	}
	if (!bSettingsResolved || DeltaTime <= 0.0f || MissionConfidence <= 0.0f
		|| UGameplayStatics::GetGlobalTimeDilation(this) < 0.001f)
	{
		return;
	}

	// The HUD clock advances one game minute per simulation second.
	SetMissionConfidence(MissionConfidence - DecayPerGameHour * DeltaTime / 60.0f);
}

void UMissionConfidenceSubsystem::AddMissionConfidence(float Amount)
{
	SetMissionConfidence(MissionConfidence + Amount);
}

void UMissionConfidenceSubsystem::SetMissionConfidence(float NewConfidence)
{
	const float Clamped = FMath::Clamp(NewConfidence, 0.0f, 100.0f);
	if (FMath::IsNearlyEqual(Clamped, MissionConfidence))
	{
		return;
	}
	const float Delta = Clamped - MissionConfidence;
	MissionConfidence = Clamped;
	OnMissionConfidenceChanged.Broadcast(MissionConfidence, Delta);
}

void UMissionConfidenceSubsystem::ResolvePlanetSettings()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<APlanetWeatherManager> It(World); It; ++It)
	{
		if (const UPlanetDefinition* Definition = It->GetPlanetDefinition())
		{
			MissionConfidence = FMath::Clamp(Definition->MissionConfidence.InitialConfidence, 0.0f, 100.0f);
			DecayPerGameHour = FMath::Max(0.0f, Definition->MissionConfidence.DecayPerGameHour);
			bSettingsResolved = true;
			OnMissionConfidenceChanged.Broadcast(MissionConfidence, 0.0f);
			return;
		}
	}
}
