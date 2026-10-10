#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MerchantDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

/** An item merchant, independent of the factions offering delivery contracts. */
UCLASS(BlueprintType)
class SURVIVETHEPLANET_API UMerchantDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant", meta = (MultiLine = "true"))
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant")
	TObjectPtr<UTexture2D> Portrait;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Portrait")
	FVector2D PortraitUVMin = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Portrait")
	FVector2D PortraitUVMax = FVector2D(1.0, 1.0);
	/** Blender-authored visual parts for this merchant's own ship. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship")
	TObjectPtr<UStaticMesh> ShipMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship")
	TObjectPtr<UStaticMesh> LandingGearMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship")
	TObjectPtr<UStaticMesh> LoadingRampMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship")
	FVector LoadingRampPivot = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship")
	TArray<FVector> ThrusterLocations;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant|Ship", meta = (ClampMin = "0.1"))
	float ShipScale = 1.0f;
	/** Item IDs from DA_TradeCatalog. An empty list offers the whole catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Merchant")
	TArray<FName> OfferedItemIds;
};

USTRUCT(BlueprintType)
struct FMerchantScheduledVisit
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visits")
	TObjectPtr<UMerchantDefinition> Merchant;
	/** Game hours after Cargo Bay completion, within the repeat cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visits", meta = (ClampMin = "0.0"))
	float ArrivalAfterHours = 24.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visits", meta = (ClampMin = "0.1"))
	float StayHours = 8.0f;
};

USTRUCT(BlueprintType)
struct FMerchantVisitSchedule
{
	GENERATED_BODY()
	/** The authored sequence repeats every eight days by default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visits", meta = (ClampMin = "0.1"))
	float RepeatCycleDays = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visits", meta = (TitleProperty = "Merchant"))
	TArray<FMerchantScheduledVisit> Visits;
};

USTRUCT(BlueprintType)
struct FMerchantVisitState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	bool bCargoBayReady = false;
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	bool bPresent = false;
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	TObjectPtr<UMerchantDefinition> Merchant;
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	bool bShipDocked = false;
	/** Time to arrival, or time to departure when present. */
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	double RemainingGameMinutes = 0.0;
	UPROPERTY(BlueprintReadOnly, Category = "Visits")
	float ArrivalProgress = 0.0f;
};
