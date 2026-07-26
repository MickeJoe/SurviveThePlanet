#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ResourceManager.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Energy UMETA(DisplayName = "Energy"),
	Iron UMETA(DisplayName = "Iron"),
	ControlChip UMETA(DisplayName = "Control Chip")
};

/** A resource and the amount required for a purchase or construction. */
USTRUCT(BlueprintType)
struct FResourceCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	EResourceType Resource = EResourceType::Iron;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources", meta = (ClampMin = "0", UIMin = "0"))
	int32 Cost = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FResourceAmountChangedSignature,
	EResourceType, ResourceType,
	int32, NewAmount);

UCLASS()
class SURVIVETHEPLANET_API AResourceManager : public AActor
{
	GENERATED_BODY()

public:
	AResourceManager();

	/** Returns the current amount of a resource. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetResourceAmount(EResourceType ResourceType) const;

	/** Replaces the current amount. Negative values are clamped to zero. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetResourceAmount(EResourceType ResourceType, int32 NewAmount);

	/** Adds to the current amount. The result is never allowed below zero. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddResource(EResourceType ResourceType, int32 Amount);

	/** Returns true when at least Amount units are available. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool HasResource(EResourceType ResourceType, int32 Amount) const;

	/** Spends Amount units if available and reports whether it succeeded. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool TrySpendResource(EResourceType ResourceType, int32 Amount);

	/** Returns true when all costs can be paid. Duplicate resource rows are combined. */
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanAffordCosts(const TArray<FResourceCost>& Costs) const;

	/** Atomically pays all costs. Nothing is spent unless every cost can be paid. */
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool TrySpendCosts(const TArray<FResourceCost>& Costs);

	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FResourceAmountChangedSignature OnResourceAmountChanged;

protected:
	virtual void BeginPlay() override;

	/** Resource amounts copied into the runtime inventory when play starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources", meta = (ClampMin = "0", UIMin = "0"))
	TMap<EResourceType, int32> InitialResourceAmounts;

	/** Current resource amounts during play. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Resources")
	TMap<EResourceType, int32> ResourceAmounts;
};
