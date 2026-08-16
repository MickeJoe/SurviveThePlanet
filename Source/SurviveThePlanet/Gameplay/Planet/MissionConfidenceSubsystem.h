#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MissionConfidenceSubsystem.generated.h"

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

	UPROPERTY(BlueprintAssignable, Category = "Mission Confidence")
	FMissionConfidenceChangedSignature OnMissionConfidenceChanged;

private:
	float MissionConfidence = 100.0f;
	float DecayPerGameHour = 5.0f;
	bool bSettingsResolved = false;

	void ResolvePlanetSettings();
};
