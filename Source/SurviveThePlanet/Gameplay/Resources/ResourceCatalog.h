#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "ResourceCatalog.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EResourceCategory : uint8
{
    RawMaterials,
    Materials,
    Components,
    AdvancedGoods,
    Energy
};

USTRUCT(BlueprintType)
struct FResourceDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    EResourceType ResourceType = EResourceType::Energy;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    EResourceCategory Category = EResourceCategory::RawMaterials;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    FName WidgetPrefix;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    FText ShortName;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Resources")
    bool bImported = false;
};

/** Shared inventory metadata for the HUD, build costs and trading. */
UCLASS()
class SURVIVETHEPLANET_API UResourceCatalog : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static const TArray<FResourceDefinition>& GetDefinitions();

    UFUNCTION(BlueprintPure, Category = "Resources")
    static TArray<FResourceDefinition> GetResourceDefinitions();

    UFUNCTION(BlueprintPure, Category = "Resources")
    static UTexture2D* GetResourceIcon(EResourceType ResourceType);
};
