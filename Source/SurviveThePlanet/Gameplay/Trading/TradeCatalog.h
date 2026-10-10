#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "TradeCatalog.generated.h"

class ABaseDrone;
class UTexture2D;

UENUM(BlueprintType)
enum class ETradeItemKind : uint8
{
	Resource,
	Blueprint,
	Drone
};

USTRUCT(BlueprintType)

struct FTradeItemDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	ETradeItemKind Kind = ETradeItemKind::Resource;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading", meta = (MultiLine = "true"))
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	EResourceType Resource = EResourceType::Iron;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	ESTPBuildTool BuildTool = ESTPBuildTool::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	TSubclassOf<ABaseDrone> DroneClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "0"))
	int32 StartingStock = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "1"))
	int32 BuyPrice = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "0"))
	int32 SellPrice = 5;
};

/** Authored prices and stock for the initial trader market. */
UCLASS(BlueprintType)

class SURVIVETHEPLANET_API UTradeCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading", meta = (TitleProperty = "DisplayName"))
	TArray<FTradeItemDefinition> Items;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trading")
	TObjectPtr<UTexture2D> MerchantPortrait;
};
