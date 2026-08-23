#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FogOfWarSubsystem.generated.h"

class ABaseBuilding;
class ABaseResourceSource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceDiscoveredSignature, ABaseResourceSource*, Resource);

/** Gameplay authority for visibility and exploration. Rendering consumes the same reveal data. */
UCLASS()
class SURVIVETHEPLANET_API UFogOfWarSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	UFUNCTION(BlueprintPure, Category = "Exploration|Fog of War") bool IsWorldLocationVisible(FVector WorldLocation) const;
	UFUNCTION(BlueprintPure, Category = "Exploration|Fog of War") bool IsResourceDiscovered(const ABaseResourceSource* Resource) const;
	UFUNCTION(BlueprintCallable, Category = "Exploration|Fog of War") void RefreshVisibility();
	UFUNCTION(BlueprintPure, Category = "Exploration|Fog of War") TArray<ABaseResourceSource*> GetDiscoveredResources() const;

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Fog of War") FResourceDiscoveredSignature OnResourceDiscovered;

private:
	void RefreshRevealSources();
	void RefreshResources();
	UPROPERTY(Transient) TArray<TObjectPtr<ABaseBuilding>> RevealSources;
	UPROPERTY(Transient) TSet<TObjectPtr<ABaseResourceSource>> DiscoveredResources;
	float RefreshAccumulator = 0.0f;
};
