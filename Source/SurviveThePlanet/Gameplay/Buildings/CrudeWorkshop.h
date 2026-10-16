#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "CrudeWorkshop.generated.h"

class UResourceProductionComponent;
class UIndustrialEffectsComponent;

UENUM(BlueprintType)
enum class ECrudeWorkshopRecipe : uint8 { Concrete, Steel, Copper };

/** A drone-operated starter workshop with a small, fixed set of basic recipes. */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API ACrudeWorkshop : public ABaseBuilding
{
	GENERATED_BODY()
public:
	ACrudeWorkshop();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetPlacementPreview(bool bPreview) override;
	virtual void SetProductionEnabled(bool bEnabled) override;
	UFUNCTION(BlueprintPure, Category = "Workshop") bool IsProducing() const { return bIsProducing; }
	UFUNCTION(BlueprintCallable, Category = "Workshop") bool SetRecipe(ECrudeWorkshopRecipe Recipe);
	UFUNCTION(BlueprintPure, Category = "Workshop") ECrudeWorkshopRecipe GetSelectedRecipe() const { return SelectedRecipe; }
	UFUNCTION(BlueprintPure, Category = "Workshop") TArray<FResourceCost> GetRecipeInputs() const;
	UFUNCTION(BlueprintPure, Category = "Workshop") EResourceType GetRecipeOutput() const;
	UFUNCTION(BlueprintPure, Category = "Workshop") float GetCycleSeconds() const;
	static EResourceType GetOutputForRecipe(ECrudeWorkshopRecipe Recipe);
	float GetCycleSecondsForRecipe(ECrudeWorkshopRecipe Recipe) const;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "0.1")) float ConcreteCycleSeconds = 120.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "0.1")) float SteelCycleSeconds = 180.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "0.1")) float CopperCycleSeconds = 150.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "1")) int32 StonePerConcrete = 4;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "1")) int32 IronPerSteel = 4;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Recipes", meta = (ClampMin = "1")) int32 OrePerCopper = 3;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UResourceProductionComponent> Production;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UIndustrialEffectsComponent> Effects;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WorkshopFan;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> WorkshopRam;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Visuals") TObjectPtr<UStaticMesh> FanMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Workshop|Visuals") TObjectPtr<UStaticMesh> RamMesh;
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Workshop") ECrudeWorkshopRecipe SelectedRecipe = ECrudeWorkshopRecipe::Concrete;

private:
	bool bIsProducing = false;
	float WorkingSeconds = 0.0f;
};
