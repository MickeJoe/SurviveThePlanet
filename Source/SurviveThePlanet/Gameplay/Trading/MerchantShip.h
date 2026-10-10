#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MerchantShip.generated.h"

class ACargoBay;
class UStaticMesh;
class UMerchantDefinition;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** The visible ship for one scheduled merchant visit. Flight and effects follow
 * the colony simulation speed and pause. */
UCLASS()
class SURVIVETHEPLANET_API AMerchantShip : public AActor
{
	GENERATED_BODY()
public:
	AMerchantShip();
	virtual void Tick(float DeltaSeconds) override;
	void Arrive(UMerchantDefinition* Definition, ACargoBay* CargoBay);
	void Depart();
	UFUNCTION(BlueprintPure, Category = "Trading|Ship")
	bool IsDocked() const { return Phase == EFlightPhase::Docked; }
	UFUNCTION(BlueprintPure, Category = "Trading|Ship")
	UMerchantDefinition* GetMerchant() const { return Merchant; }

private:
	enum class EFlightPhase : uint8 { Arriving, Docked, Departing };
	EFlightPhase Phase = EFlightPhase::Arriving;
	float FlightElapsed = 0.0f;
	float VisualElapsed = 0.0f;
	FTransform DockTransform;
	FVector DepartureStart;
	FRotator DepartureRotation;
	UPROPERTY(Transient) TObjectPtr<UMerchantDefinition> Merchant;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Hull;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> LandingGear;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> LoadingRamp;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Exhausts;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> DustClouds;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ExhaustMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> DustMaterials;
	UStaticMeshComponent* AddVisual(FName Name, UStaticMesh* Mesh);
	void UpdateFlight(float DeltaSeconds);
	void UpdateEffects(float Thrust, float NearDeck);
};
