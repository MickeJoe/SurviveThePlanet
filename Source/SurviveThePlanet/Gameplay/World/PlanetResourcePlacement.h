#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "PlanetResourcePlacement.generated.h"

class AHexSectorGrid;

USTRUCT(BlueprintType)
struct FPlanetResourceRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EResourceType ResourceType = EResourceType::Iron;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SlotType = TEXT("Mineral");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MinQuantity = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxQuantity = 1000;
	/** Deposit footprint radius, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinDistanceFromHQ = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxDistanceFromHQ = 1000000;
	/** Edge-to-edge clearance from all other deposits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinimumSpacing = 200;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> RequiredTemplateTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> AllowedSectorIds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bGuaranteed = true;
};

UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UPlanetResourceDistribution : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPlanetResourceRule> Rules;
};

USTRUCT(BlueprintType)
struct FPlanetPlacedDeposit
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName RuleId;
	UPROPERTY(BlueprintReadOnly) EResourceType ResourceType = EResourceType::Iron;
	UPROPERTY(BlueprintReadOnly) int32 SectorId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
	UPROPERTY(BlueprintReadOnly) float Radius = 0;
	UPROPERTY(BlueprintReadOnly) float MinimumSpacing = 0;
};

USTRUCT(BlueprintType)
struct FPlanetSlotDiagnostic
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName RuleId;
	UPROPERTY(BlueprintReadOnly) int32 SectorId = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) int32 SlotIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FString Reason;
};

USTRUCT(BlueprintType)
struct FPlanetResourcePlacementResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 GenerationVersion = 1;
	UPROPERTY(BlueprintReadOnly) int32 LayoutSeed = 0;
	UPROPERTY(BlueprintReadOnly) int32 PlacementSeed = 0;
	UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly) TArray<FPlanetPlacedDeposit> Deposits;
	UPROPERTY(BlueprintReadOnly) TArray<FPlanetSlotDiagnostic> Slots;
	UPROPERTY(BlueprintReadOnly) TArray<FString> Errors;
	UPROPERTY(BlueprintReadOnly) TArray<FString> Warnings;
};

/** Optional inspection component. Generation is explicit and never rebuilds the layout or spawns deposits. */
UCLASS(ClassGroup=(World), meta=(BlueprintSpawnableComponent))
class SURVIVETHEPLANET_API UPlanetResourcePlacementComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPlanetResourcePlacementComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UPlanetResourceDistribution> Distribution;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PlacementSeed = 42;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bShowDebug = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DebugRuleId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FPlanetResourcePlacementResult Result;
	UFUNCTION(BlueprintCallable, CallInEditor) void Generate();
	UFUNCTION(BlueprintCallable, Category="Planet|Resources")
	static FPlanetResourcePlacementResult PlaceResources(const AHexSectorGrid* Grid, const UPlanetResourceDistribution* Input, int32 Seed);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
