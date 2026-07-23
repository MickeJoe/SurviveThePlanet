#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConstructionJobQueueSubsystem.generated.h"

class ABaseBuilding;

USTRUCT(BlueprintType)
struct FSTPConstructionJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ABaseBuilding> TargetBuilding = nullptr;
};

UCLASS()
class SURVIVETHEPLANET_API UConstructionJobQueueSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Construction Jobs")
	void EnqueueConstructionJob(ABaseBuilding* Building);

	UFUNCTION(BlueprintCallable, Category = "Construction Jobs")
	bool TryDequeueConstructionJob(FSTPConstructionJob& OutJob);

	UFUNCTION(BlueprintPure, Category = "Construction Jobs")
	int32 GetJobCount() const { return Jobs.Num(); }

private:
	UPROPERTY()
	TArray<FSTPConstructionJob> Jobs;
};