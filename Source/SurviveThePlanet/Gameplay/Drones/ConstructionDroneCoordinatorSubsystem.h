#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConstructionDroneCoordinatorSubsystem.generated.h"

class ABaseDrone;
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
	void RegisterDrone(ABaseDrone* Drone);

	UFUNCTION(BlueprintCallable, Category = "Construction Drones")
	void UnregisterDrone(ABaseDrone* Drone);

	UFUNCTION(BlueprintPure, Category = "Construction Drones")
	int32 GetRegisteredDroneCount() const { return Drones.Num(); }

private:
	UPROPERTY()
	TArray<TObjectPtr<ABaseDrone>> Drones;

	void RemoveInvalidDrones();
	void EnsureDroneHasIdleDestination(ABaseDrone* Drone);
	ABaseBuilding* FindBaseModule() const;
	APlanetSurfaceManager* FindPlanetSurfaceManager() const;
};
