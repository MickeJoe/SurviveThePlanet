#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConstructionDroneCoordinatorSubsystem.generated.h"

class AConstructionDrone;
class ABaseBuilding;
class APlanetSurfaceManager;

UCLASS()
class SURVIVETHEPLANET_API UConstructionDroneCoordinatorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Construction Drones")
	void RegisterConstructionDrone(AConstructionDrone* Drone);

	UFUNCTION(BlueprintCallable, Category = "Construction Drones")
	void UnregisterConstructionDrone(AConstructionDrone* Drone);

	UFUNCTION(BlueprintPure, Category = "Construction Drones")
	int32 GetRegisteredDroneCount() const { return ConstructionDrones.Num(); }

private:
	UPROPERTY()
	TArray<TObjectPtr<AConstructionDrone>> ConstructionDrones;

	void RemoveInvalidDrones();
	void EnsureDroneHasIdleDestination(AConstructionDrone* Drone);
	ABaseBuilding* FindBaseModule() const;
	APlanetSurfaceManager* FindPlanetSurfaceManager() const;
};