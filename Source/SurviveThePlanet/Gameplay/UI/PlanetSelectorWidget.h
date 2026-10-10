#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlanetSelectorWidget.generated.h"
class UButton;
class UTextBlock;
class UPlanetDefinition;
UCLASS()
class SURVIVETHEPLANET_API UPlanetSelectorWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UButton> NexarisButton;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UButton> AridusButton;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UButton> BorealisButton;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UButton> LandButton;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UTextBlock> PlanetName;
	UPROPERTY(BlueprintReadOnly, Category="Planet Selection", meta=(BindWidget)) TObjectPtr<UTextBlock> PlanetDetails;
	UPROPERTY(Transient) TObjectPtr<UPlanetDefinition> SelectedPlanet;
private:
	void SelectPlanet(const TCHAR* AssetPath);
	UFUNCTION() void SelectNexaris();
	UFUNCTION() void SelectAridus();
	UFUNCTION() void SelectBorealis();
	UFUNCTION() void Land();
};
