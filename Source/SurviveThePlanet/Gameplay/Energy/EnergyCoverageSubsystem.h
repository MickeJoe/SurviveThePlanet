#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnergyCoverageSubsystem.generated.h"

class UEnergyCoverageComponent;

/** Event-driven registry of coverage sources; no world scan or tick is needed. */
UCLASS()
class SURVIVETHEPLANET_API UEnergyCoverageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	void RegisterSource(UEnergyCoverageComponent* Source);
	void UnregisterSource(UEnergyCoverageComponent* Source);
	void RefreshSource(UEnergyCoverageComponent* Source);
	void SetCoverageVisualizationVisible(bool bVisible);
	UEnergyCoverageComponent* FindNearestSource(const FVector& Location, const AActor* ExcludedOwner, UEnergyCoverageComponent* CurrentSource = nullptr, float SwitchMargin = 0) const;
	bool IsLocationConnectedToPowerGrid(const FVector& Location) const;
	bool IsSourceConnectedToPowerGrid(const UEnergyCoverageComponent* Source) const;
	void RefreshPoweredCoverageVisualization();
	uint32 GetSourceRevision() const { return SourceRevision; }
	FSimpleMulticastDelegate OnSourcesChanged;

private:
	TArray<TWeakObjectPtr<UEnergyCoverageComponent>> Sources;
	bool bVisualizationVisible = false;
	uint32 SourceRevision = 0;
};
