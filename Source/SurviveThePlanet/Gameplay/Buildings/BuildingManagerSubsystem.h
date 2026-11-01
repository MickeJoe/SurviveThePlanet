#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "BuildingManagerSubsystem.generated.h"

class ABaseBuilding;
class UBuildingCatalogDataAsset;
class UBuildingDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildingCatalogChangedSignature, UBuildingCatalogDataAsset*, Catalog);

/** World-scoped registry for building definitions and their actor classes. */
UCLASS()
class SURVIVETHEPLANET_API UBuildingManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Extraction-only unlocks must never create an ordinary ground-placement preview. */
	static bool IsStandalonePlacementEnabled(ESTPBuildTool Tool) { return Tool != ESTPBuildTool::SilicaExtractor; }
	/** Identifies a silica extractor without a deposit; deposit-bound mines remain valid. */
	static bool IsRetiredStandaloneBuilding(const ABaseBuilding* Building);
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Buildings")
	void SetCatalog(UBuildingCatalogDataAsset* NewCatalog);

	UFUNCTION(BlueprintPure, Category = "Buildings")
	UBuildingCatalogDataAsset* GetCatalog() const { return Catalog; }

	UFUNCTION(BlueprintPure, Category = "Buildings")
	UBuildingDataAsset* GetDefinition(ESTPBuildTool Tool) const;

	UFUNCTION(BlueprintPure, Category = "Buildings")
	TSubclassOf<ABaseBuilding> GetBuildingClass(ESTPBuildTool Tool) const;

	UFUNCTION(BlueprintCallable, Category = "Buildings")
	void SetBuildingClassOverride(ESTPBuildTool Tool, TSubclassOf<ABaseBuilding> BuildingClass);

	UFUNCTION(BlueprintPure, Category = "Buildings")
	TArray<UBuildingDataAsset*> GetToolbarDefinitions() const;

	UFUNCTION(BlueprintPure, Category = "Buildings")
	TArray<UBuildingDataAsset*> GetAllDefinitions() const;

	UPROPERTY(BlueprintAssignable, Category = "Buildings")
	FBuildingCatalogChangedSignature OnCatalogChanged;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBuildingCatalogDataAsset> Catalog;

	UPROPERTY(Transient)
	TMap<ESTPBuildTool, TObjectPtr<UBuildingDataAsset>> DefinitionsByTool;

	UPROPERTY(Transient)
	TMap<ESTPBuildTool, TSubclassOf<ABaseBuilding>> RuntimeClassOverrides;

	void RebuildIndex();
	void LoadDefaultCatalog();
};
