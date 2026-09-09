#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Steelworks.generated.h"

class AResourceManager;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteelworksProductionStateChangedSignature, bool, bIsProducing);

/** Converts iron into steel and drives a readable active-production visual cycle. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ASteelworks : public ABaseBuilding
{
	GENERATED_BODY()

public:
	ASteelworks();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetPlacementPreview(bool bPreview) override;
	virtual void SetPlacementPreviewValid(bool bValidPlacement) override;

	UFUNCTION(BlueprintPure, Category = "Steelworks|Production")
	bool IsProducing() const { return bIsProducing; }

	UFUNCTION(BlueprintPure, Category = "Steelworks|Production")
	float GetSteelProductionPerMinute() const;

	UFUNCTION(BlueprintPure, Category = "Steelworks|Production")
	float GetIronConsumptionPerMinute() const;

	UPROPERTY(BlueprintAssignable, Category = "Steelworks|Production")
	FSteelworksProductionStateChangedSignature OnProductionStateChanged;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> ProcessDrumA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> ProcessDrumB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> FurnaceGlowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> SteelSlabA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> SteelSlabB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UStaticMeshComponent> SteelSlabC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UPointLightComponent> FurnaceLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UNiagaraComponent> SparkFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UNiagaraComponent> SteamFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Steelworks|Components")
	TObjectPtr<UNiagaraComponent> SmokeFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UStaticMesh> ProcessDrumMeshAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UStaticMesh> SteelSlabMeshAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UStaticMesh> FurnaceGlowMeshAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UNiagaraSystem> SparkSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UNiagaraSystem> SteamSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visual Assets")
	TObjectPtr<UNiagaraSystem> SmokeSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visuals", meta = (ClampMin = "0.0"))
	float DrumRotationDegreesPerSecond = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steelworks|Visuals", meta = (ClampMin = "0.1"))
	float VisualLoopSeconds = 4.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> CachedResourceManager;

	bool bIsProducing = false;
	bool bVisualStateInitialized = false;
	float CycleProgressSeconds = 0.0f;
	float VisualTimeSeconds = 0.0f;
	float SparkAccumulatorSeconds = 0.0f;

	const USteelworksBuildingDataAsset* GetSteelworksData() const;
	AResourceManager* ResolveResourceManager();
	void ApplyVisualAssets();
	void SetProductionActive(bool bNewActive);
	void UpdateActiveVisuals(float DeltaSeconds);
	void RefreshPreviewVisuals();
};
