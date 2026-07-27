#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "MiningMachine.generated.h"

class ABaseResourceSource;

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

	UFUNCTION(BlueprintCallable, Category = "Mining Machine")
	bool AttachToResourceSource(ABaseResourceSource* NewResourceSource);

	UFUNCTION(BlueprintPure, Category = "Mining Machine")
	bool CanMineResourceSource(const ABaseResourceSource* CandidateSource) const;

	UFUNCTION(BlueprintPure, Category = "Mining Machine")
	ABaseResourceSource* GetResourceSource() const { return ResourceSource; }

	/** Source transform plus an optional art-alignment offset. */
	UFUNCTION(BlueprintPure, Category = "Mining Machine|Placement")
	FTransform GetPlacementTransformForSource(const ABaseResourceSource* CandidateSource) const;

	UFUNCTION(BlueprintCallable, Category = "Mining Machine|Placement")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintCallable, Category = "Mining Machine|Placement")
	void SetPlacementPreviewValid(bool bValidPlacement);

	/** Changes which source mesh is temporarily replaced by this preview. */
	void SetPreviewResourceSource(ABaseResourceSource* NewPreviewSource);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Resource types accepted by this machine. Defaults to Iron. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mining Machine")
	TArray<EResourceType> SupportedResourceTypes;

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

	void RefreshPlacementPreviewVisual();
};
