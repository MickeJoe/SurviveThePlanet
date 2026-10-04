#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "CoalMiningMachine.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

/** Compact coal cutter. All construction, power and extraction rules live in MiningMachine. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ACoalMiningMachine : public AMiningMachine
{
	GENERATED_BODY()
public:
	ACoalMiningMachine();
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coal|Visuals")
	TObjectPtr<UStaticMeshComponent> Cutter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coal|Visuals")
	TObjectPtr<UInstancedStaticMeshComponent> ConveyorCoal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coal|Visuals")
	TObjectPtr<UInstancedStaticMeshComponent> CuttingChips;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coal|Visuals")
	TObjectPtr<UInstancedStaticMeshComponent> CuttingDust;
private:
	float OperatingTime = 0.0f;
	void InitializeOperatingVisuals();
	void UpdateOperatingVisuals(bool bOperating);
};
