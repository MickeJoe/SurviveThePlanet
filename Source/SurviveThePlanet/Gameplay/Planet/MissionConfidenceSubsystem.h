#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MissionConfidenceSubsystem.generated.h"

UENUM(BlueprintType)
enum class EMissionOutcome : uint8 { InProgress, Complete, Failed };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMissionEndedSignature, EMissionOutcome, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionProgressChangedSignature, int32, Progress, int32, Target);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionConfidenceChangedSignature, float, NewConfidence, float, Delta);

/** Owns the current 0-100 mission confidence and its planet-configured decay. */
UCLASS()
class SURVIVETHEPLANET_API UMissionConfidenceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintPure, Category = "Mission Confidence")
	float GetMissionConfidence() const { return MissionConfidence; }

	UFUNCTION(BlueprintPure, Category = "Mission Confidence")
	float GetDecayPerGameHour() const { return DecayPerGameHour; }

	UFUNCTION(BlueprintCallable, Category = "Mission Confidence")
	void AddMissionConfidence(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Mission Confidence")
	void SetMissionConfidence(float NewConfidence);

	UFUNCTION(BlueprintPure, Category = "Mission Confidence")
	bool IsDecayPaused() const { return bDecayPaused; }

	UFUNCTION(BlueprintCallable, Category = "Mission Confidence")
	void SetDecayPaused(bool bPaused);

	UPROPERTY(BlueprintAssignable, Category = "Mission Confidence")
	FMissionConfidenceChangedSignature OnMissionConfidenceChanged;


	UFUNCTION(BlueprintPure, Category = "Mission")
	EMissionOutcome GetMissionOutcome() const { return Outcome; }
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetMissionProgress() const { return MissionProgress; }
	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetMissionProgressTarget() const { return 10; }
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void AddMissionProgress(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void SetMissionProgress(int32 Amount);
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FMissionEndedSignature OnMissionEnded;
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FMissionProgressChangedSignature OnMissionProgressChanged;
	float GetMissionElapsedSeconds() const { return MissionElapsedSeconds; }

private:
	EMissionOutcome Outcome = EMissionOutcome::InProgress;
	int32 MissionProgress = 0;
	float MissionElapsedSeconds = 0.0f;
	void EvaluateMissionOutcome();
	float MissionConfidence = 100.0f;
	float DecayPerGameHour = 5.0f;
	bool bSettingsResolved = false;
	bool bDecayPaused = false;

	void ResolvePlanetSettings();
};
