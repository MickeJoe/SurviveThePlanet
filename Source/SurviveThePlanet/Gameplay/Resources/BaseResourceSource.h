#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "BaseResourceSource.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FResourceSourceAmountChangedSignature, int32, RemainingAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FResourceSourceDepletedSignature);

/** Base actor for a finite resource deposit occupying one planet grid cell. */
UCLASS(Abstract, Blueprintable)
class SURVIVETHEPLANET_API ABaseResourceSource : public AActor
{
	GENERATED_BODY()

public:
	ABaseResourceSource();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "Resource Source")
	EResourceType GetResourceType() const { return ResourceType; }

	UFUNCTION(BlueprintPure, Category = "Resource Source")
	int32 GetStartingAmount() const { return StartingAmount; }

	UFUNCTION(BlueprintPure, Category = "Resource Source")
	int32 GetRemainingAmount() const { return RemainingAmount; }

	UFUNCTION(BlueprintPure, Category = "Resource Source")
	FSTPGridCell GetGridCell() const { return GridCell; }

	/** Removes up to RequestedAmount and returns the amount actually extracted. */
	UFUNCTION(BlueprintCallable, Category = "Resource Source")
	int32 ExtractResource(int32 RequestedAmount);

	UPROPERTY(BlueprintAssignable, Category = "Resource Source")
	FResourceSourceAmountChangedSignature OnRemainingAmountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Resource Source")
	FResourceSourceDepletedSignature OnDepleted;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource Source|Components")
	TObjectPtr<UStaticMeshComponent> ResourceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource Source")
	EResourceType ResourceType = EResourceType::Iron;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource Source", meta = (ClampMin = "0", UIMin = "0"))
	int32 StartingAmount = 100;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resource Source", meta = (ClampMin = "0", UIMin = "0"))
	int32 RemainingAmount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resource Source|Grid")
	FSTPGridCell GridCell;

private:
	void RefreshGridCell();
	APlanetSurfaceManager* FindSurfaceManager() const;

	UPROPERTY(Transient)
	TObjectPtr<APlanetSurfaceManager> SurfaceManager;
};
