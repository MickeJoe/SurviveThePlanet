#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "MiningMachine.generated.h"

class ABaseResourceSource;
class APlanetSurfaceManager;
struct FSTPGridPlacement;

/**
 * Constructible mining installation that replaces a resource deposit visually.
 * The machine always snaps to the source transform and occupies the source's
 * already-reserved area instead of claiming additional land beside it.
 */
UCLASS(Blueprintable)
class SURVIVETHEPLANET_API AMiningMachine : public ABaseBuilding
{
	GENERATED_BODY()

public:
	AMiningMachine();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Mining Machine")
	bool AttachToResourceSource(ABaseResourceSource* NewResourceSource);

	UFUNCTION(BlueprintPure, Category = "Mining Machine")
	bool CanMineResourceSource(const ABaseResourceSource* CandidateSource) const;

	UFUNCTION(BlueprintPure, Category = "Mining Machine")
	ABaseResourceSource* GetResourceSource() const { return ResourceSource; }

	UFUNCTION(BlueprintPure, Category = "Mining Machine|Output")
	float GetOutputPerMinuteAt100Percent(EResourceType ResourceType) const;

	UFUNCTION(BlueprintPure, Category = "Mining Machine|Output")
	float GetCurrentOutputPerMinute() const;

	/** Shared production gate for extraction, displayed output and operating visuals. */
	UFUNCTION(BlueprintPure, Category = "Mining Machine|Output")
	bool IsProducingResource() const;

	/** Mining machines only draw power while they have a valid source and productive drones. */
	virtual float GetEnergyConsumptionPerMinute() const override;

	/** Source transform plus an optional art-alignment offset. */
	UFUNCTION(BlueprintPure, Category = "Mining Machine|Placement")
	FTransform GetPlacementTransformForSource(const ABaseResourceSource* CandidateSource) const;

	/** Evaluates a future source transform without spawning or reserving a deposit. */
	UFUNCTION(BlueprintPure, Category = "Mining Machine|Placement")
	FTransform GetPlacementTransformForSourceAtTransform(const ABaseResourceSource* CandidateSource, const FTransform& SourceTransform) const;

	bool CanBuildAtSourceTransform(const ABaseResourceSource* CandidateSource, const FTransform& SourceTransform,
		APlanetSurfaceManager* Surface, FSTPGridPlacement& OutPlacement, int32 ExtraTerrainMarginCells = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Mining Machine|Placement")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintCallable, Category = "Mining Machine|Placement")
	void SetPlacementPreviewValid(bool bValidPlacement);

	/** Changes which source mesh is temporarily replaced by this preview. */
	void SetPreviewResourceSource(ABaseResourceSource* NewPreviewSource);
	ABaseResourceSource* GetPreviewResourceSource() const { return PreviewResourceSource; }
	bool IsMiningPlacementPreviewValid() const { return bPlacementPreviewValid; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resource types accepted by this machine. Defaults to Iron. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining Machine")
	TArray<EResourceType> SupportedResourceTypes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining Machine|Output", meta = (TitleProperty = "Resource"))
	TArray<FSTPResourceOutputRate> OutputPerDroneAt100Percent;

	/** Art-only alignment relative to the resource actor transform. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining Machine|Placement")
	FTransform SourceTransformOffset = FTransform::Identity;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mining Machine")
	TObjectPtr<ABaseResourceSource> ResourceSource;

private:
	bool bPlacementPreview = false;
	bool bPlacementPreviewValid = false;

	UPROPERTY(Transient)
	TObjectPtr<ABaseResourceSource> PreviewResourceSource;
	float PendingResourceOutput = 0.0f;

	void RefreshPlacementPreviewVisual();
	const UMiningBuildingDataAsset* GetMiningBuildingData() const;
	const TArray<EResourceType>& GetSupportedResourceTypes() const;
	const TArray<FSTPResourceOutputRate>& GetOutputRates() const;
	FTransform GetSourceTransformOffset() const;
};
