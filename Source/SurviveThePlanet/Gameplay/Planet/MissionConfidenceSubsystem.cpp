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
	if (Outcome != EMissionOutcome::InProgress) return;
	if (!bSettingsResolved)
	{
		ResolvePlanetSettings();
	}
	if (Outcome != EMissionOutcome::InProgress) return;
	EvaluateMissionOutcome();
	if (Outcome != EMissionOutcome::InProgress) return;
	if (!UGameplayStatics::IsGamePaused(this)) MissionElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (bDecayPaused || !bSettingsResolved || DeltaTime <= 0.0f || MissionConfidence <= 0.0f
		|| UGameplayStatics::GetGlobalTimeDilation(this) < 0.001f)
	{
		return;
	}

	// The HUD clock advances one game minute per simulation second.
	SetMissionConfidence(MissionConfidence - DecayPerGameHour * DeltaTime / 60.0f);
}

void UMissionConfidenceSubsystem::SetDecayPaused(bool bPaused)
{
	if (bDecayPaused == bPaused) return;
	bDecayPaused = bPaused;
	// Refresh the HUD even though the confidence amount itself is unchanged.
	OnMissionConfidenceChanged.Broadcast(MissionConfidence, 0.0f);
}

void UMissionConfidenceSubsystem::AddMissionConfidence(float Amount)
{
	SetMissionConfidence(MissionConfidence + Amount);
}

void UMissionConfidenceSubsystem::SetMissionConfidence(float NewConfidence)
{
	if (Outcome != EMissionOutcome::InProgress || !FMath::IsFinite(NewConfidence)) return;
	const float Clamped = FMath::Clamp(NewConfidence, 0.0f, 100.0f);
	if (Clamped == MissionConfidence)
	{
		return;
	}
	const float Delta = Clamped - MissionConfidence;
	MissionConfidence = Clamped;
	OnMissionConfidenceChanged.Broadcast(MissionConfidence, Delta);
	EvaluateMissionOutcome();
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

void UMissionConfidenceSubsystem::AddMissionProgress(int32 Amount)
{
	SetMissionProgress(static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(MissionProgress) + Amount, 0, GetMissionProgressTarget())));
}

void UMissionConfidenceSubsystem::SetMissionProgress(int32 Amount)
{
	if (Outcome != EMissionOutcome::InProgress) return;
	const int32 NewProgress = FMath::Clamp(Amount, 0, GetMissionProgressTarget());
	if (NewProgress == MissionProgress) return;
	MissionProgress = NewProgress;
	OnMissionProgressChanged.Broadcast(MissionProgress, GetMissionProgressTarget());
	EvaluateMissionOutcome();
}

void UMissionConfidenceSubsystem::EvaluateMissionOutcome()
{
	if (Outcome != EMissionOutcome::InProgress) return;
	// Once resolved, the outcome is immutable even if another event arrives this frame.
	if (MissionConfidence <= 0.0f) Outcome = EMissionOutcome::Failed;
	else if (MissionProgress >= GetMissionProgressTarget()) Outcome = EMissionOutcome::Complete;
	else return;
	OnMissionEnded.Broadcast(Outcome);
}
