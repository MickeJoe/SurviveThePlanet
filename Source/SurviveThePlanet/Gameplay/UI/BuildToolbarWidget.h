#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "BuildToolbarWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTexture2D;
class UWidget;

USTRUCT(BlueprintType)
struct FBuildToolButtonConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Tool")
	ESTPBuildTool Tool = ESTPBuildTool::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Tool")
	FText Tooltip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Tool")
	TObjectPtr<UTexture2D> IconTexture;
};

UCLASS(Blueprintable)
class SURVIVETHEPLANET_API UBuildToolbarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBuildToolbarWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Build Toolbar")
	void SetActiveTool(ESTPBuildTool NewTool);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Toolbar")
	TArray<FBuildToolButtonConfig> Buttons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Toolbar")
	FVector2D ButtonSize = FVector2D(58.0f, 58.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Toolbar")
	FLinearColor NormalBorderColor = FLinearColor(0.05f, 0.04f, 0.025f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Toolbar")
	FLinearColor SelectedBorderColor = FLinearColor(0.15f, 1.0f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build Toolbar")
	FLinearColor EmptyIconTint = FLinearColor(0.12f, 0.16f, 0.18f, 1.0f);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UButton> EnergyCableButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UButton> EnergyModuleButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UButton> MiningBuildingButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UImage> EnergyCableIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UImage> EnergyModuleIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UImage> MiningBuildingIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UBorder> EnergyCableBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UBorder> EnergyModuleBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Build Toolbar|Designed Widgets")
	TObjectPtr<UBorder> MiningBorder;

	UFUNCTION(BlueprintImplementableEvent, Category = "Build Toolbar")
	void BP_ActiveToolChanged(ESTPBuildTool NewTool);

private:
	UPROPERTY(Transient)
	TMap<ESTPBuildTool, TObjectPtr<UBorder>> ButtonBorders;

	UPROPERTY(Transient)
	ESTPBuildTool ActiveTool = ESTPBuildTool::None;

	void RebuildToolbar();
	UWidget* BuildButton(const FBuildToolButtonConfig& Config);
	bool HasDesignedToolbar() const;
	void BindDesignedToolbar();
	const FBuildToolButtonConfig* FindButtonConfig(ESTPBuildTool Tool) const;
	void ApplyIcon(UImage* Icon, ESTPBuildTool Tool) const;

	UFUNCTION()
	void HandleEnergyCableClicked();

	UFUNCTION()
	void HandleEnergyModuleClicked();

	UFUNCTION()
	void HandleMiningMachineClicked();

	UFUNCTION()
	void HandleControllerBuildToolChanged(ESTPBuildTool NewTool);

	void HandleToolClicked(ESTPBuildTool Tool);
	void RefreshButtonStates();
};
