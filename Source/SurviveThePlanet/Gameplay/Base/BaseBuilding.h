#pragma once

#include "CoreMinimal.h"
#include "Gameplay/SelectableWorldActor.h"
#include "BaseBuilding.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class ESTPBuildingType : uint8
{
	BaseModule UMETA(DisplayName = "Base Module"),
	EnergyModule UMETA(DisplayName = "Energy Module"),
	Other UMETA(DisplayName = "Other")
};

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ABaseBuilding : public ASelectableWorldActor
{
	GENERATED_BODY()

public:
	ABaseBuilding();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void SetConstructionProgress(float NewProgress);

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void ShowConstructionProgress();

	UFUNCTION(BlueprintCallable, Category = "Construction")
	void HideConstructionProgress();

	UFUNCTION(BlueprintPure, Category = "Construction")
	float GetConstructionProgress() const { return ConstructionProgress; }

	UFUNCTION(BlueprintPure, Category = "Grid")
	FIntPoint GetGridFootprint() const { return GridFootprint; }

	UFUNCTION(BlueprintPure, Category = "Base Building")
	ESTPBuildingType GetBuildingType() const { return BuildingType; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building|Visuals")
	TObjectPtr<UStaticMesh> BaseModuleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> ConstructionProgressBar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building")
	FName BuildingTag = TEXT("BaseModule");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building")
	ESTPBuildingType BuildingType = ESTPBuildingType::BaseModule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 1000.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Base Building")
	float CurrentHealth = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Base Building", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EnergyCapacity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ConstructionProgress = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1", UIMin = "1"))
	FIntPoint GridFootprint = FIntPoint(2, 2);

private:
	void ConfigureMesh();
	void RefreshConstructionProgressBar();
};
