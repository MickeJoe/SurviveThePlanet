#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/SaveGame.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Resources/ResourceCatalog.h"
#include "ResourceDisplayWidget.generated.h"

class AResourceManager;
class APlanetWeatherManager;
class UImage;
class UButton;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;

UCLASS()
class SURVIVETHEPLANET_API UGameTimeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	double TotalGameMinutes = 480.0;

	UPROPERTY(SaveGame)
	float TimeScale = 1.0f;

	UPROPERTY(SaveGame)
	bool bTimePaused = true;
};

USTRUCT(BlueprintType)
struct FResourceDisplayConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	EResourceType ResourceType = EResourceType::Energy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	FText Tooltip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
	TObjectPtr<UTexture2D> IconTexture;
};

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UResourceDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UResourceDisplayWidget(const FObjectInitializer& ObjectInitializer);

public:
	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void SetCheatSpeed10();
	UFUNCTION(BlueprintCallable, Category = "Cheats")
	void SetCheatSpeed30();
	void SetTradingPaused(bool bPaused);
	bool IsSimulationPaused() const { return bTimePaused || bTradingPaused; }

    UFUNCTION(BlueprintCallable, Category = "Resource Display")
    void SelectResourceCategory(EResourceCategory Category);

    UFUNCTION(BlueprintCallable, Category = "Resource Display")
    void SetResourceCardsVisible(bool bVisible);

    UFUNCTION(BlueprintPure, Category = "Resource Display")
    bool AreResourceCardsVisible() const { return bResourceCardsVisible; }

    UFUNCTION(BlueprintPure, Category = "Resource Display")
    EResourceCategory GetSelectedResourceCategory() const { return SelectedResourceCategory; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Configure labels and icon textures on a Widget Blueprint derived from this class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource Display")
	TArray<FResourceDisplayConfig> Resources;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> EnergyIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> EnergyAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> CreditsAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> IronIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> IronAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> ControlChipIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> ControlChipAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> CopperIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> CopperAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> StoneIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> StoneAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> WaterIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> WaterAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UImage> ConcreteIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Resource Display")
	TObjectPtr<UTextBlock> ConcreteAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> EnergyRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> IronRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> CopperRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> StoneRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> WaterRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> ConcreteRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UImage> SteelIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UImage> CoalIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> CoalAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> CoalRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> SteelAmountText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> SteelRateText;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Resource Display")
	TObjectPtr<UImage> PolymerIcon;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Resource Display")
	TObjectPtr<UTextBlock> PolymerAmountText;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Resource Display")
	TObjectPtr<UTextBlock> PolymerRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UImage> ConnectorIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> ConnectorAmountText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Resource Display")
	TObjectPtr<UTextBlock> ConnectorRateText;

private:
    EResourceCategory SelectedResourceCategory = EResourceCategory::RawMaterials;
    bool bResourceCardsVisible = true;

    void ResolveCategoryWidgets();
    void RefreshCategoryDisplay();
    UTextBlock* GetResourceRateText(EResourceType ResourceType) const;

    UFUNCTION()
    void HandleRawMaterialsClicked();
    UFUNCTION()
    void HandleMaterialsClicked();
    UFUNCTION()
    void HandleComponentsClicked();
    UFUNCTION()
    void HandleAdvancedGoodsClicked();
    UFUNCTION()
    void HandleToggleResourceCardsClicked();

	UPROPERTY(Transient)
	TObjectPtr<AResourceManager> ResourceManager;

	UPROPERTY(Transient)
	TObjectPtr<APlanetWeatherManager> PlanetWeatherManager;

	float RateRefreshAccumulator = 0.0f;
	float TimeSaveAccumulator = 0.0f;
	double TotalGameMinutes = 480.0;
	float TimeScale = 1.0f;
	bool bTimePaused = true;
	bool bTradingPaused = false;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ClockText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PhaseText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WindText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RainText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CloudText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PauseTimeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Speed1Button;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Speed15Button;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Speed2Button;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Speed3Button;

	void ResolveResourceManager();
	void ResolvePlanetWeatherManager();
	void RefreshAllResources();
	void RefreshResourceRates();
	void ApplyConfiguredIcons();
	void ResolveTimeWidgets();
	void LoadGameTime();
	void SaveGameTime() const;
	void RefreshGameTimeDisplay();
	void RefreshTimeControlStyles();
	void ApplySimulationRate() const;
	void SetGameTimeScale(float NewTimeScale);
	UImage* GetResourceImage(EResourceType ResourceType) const;
	UTextBlock* GetResourceAmountText(EResourceType ResourceType) const;

	UFUNCTION()
	void HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount);

	UFUNCTION()
	void HandleCreditsChanged(int32 NewCredits);

	UFUNCTION()
	void HandlePlanetWeatherChanged(FPlanetWeatherState NewWeather);

	UFUNCTION()
	void HandlePauseTimeClicked();

	UFUNCTION()
	void HandleSpeed1Clicked();

	UFUNCTION()
	void HandleSpeed15Clicked();

	UFUNCTION()
	void HandleSpeed2Clicked();

	UFUNCTION()
	void HandleSpeed3Clicked();
};
