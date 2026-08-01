#include "Gameplay/UI/BuildToolbarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "SurviveThePlanet.h"
#include "SurviveThePlanetPlayerController.h"

UBuildToolbarWidget::UBuildToolbarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Buttons = {
		{ ESTPBuildTool::EnergyCable, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyCableTooltip", "Energy Cable"), nullptr },
		{ ESTPBuildTool::EnergyModule, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyModuleTooltip", "Energy Module"), nullptr },
		{ ESTPBuildTool::EnergyStorage, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyStorageTooltip", "Build Battery Storage\nIncreases the connected grid's maximum energy capacity."), nullptr },
		{ ESTPBuildTool::MiningMachine, NSLOCTEXT("SurviveThePlanet", "BuildToolMiningMachineTooltip", "Build Mining Machine\nPlace on an available resource deposit."), nullptr }
	};
}

void UBuildToolbarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HasDesignedToolbar())
	{
		BindDesignedToolbar();
		RefreshButtonStates();
	}
	else
	{
		RebuildToolbar();
	}

	if (ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>())
	{
		Controller->OnBuildToolChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
		Controller->OnBuildToolChanged.AddDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
		SetActiveTool(Controller->GetActiveBuildTool());
	}
}

void UBuildToolbarWidget::NativeDestruct()
{
	if (ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>())
	{
		Controller->OnBuildToolChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
	}

	Super::NativeDestruct();
}

void UBuildToolbarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (HasDesignedToolbar())
	{
		RefreshButtonStates();
	}
}

void UBuildToolbarWidget::RebuildToolbar()
{
	if (!WidgetTree)
	{
		return;
	}

	ButtonBorders.Reset();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildToolbarRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UHorizontalBox* ToolbarBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildToolbarBox"));
	UCanvasPanelSlot* ToolbarSlot = RootCanvas->AddChildToCanvas(ToolbarBox);
	ToolbarSlot->SetAnchors(FAnchors(0.0f, 1.0f));
	ToolbarSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	ToolbarSlot->SetPosition(FVector2D(26.0f, -26.0f));
	ToolbarSlot->SetAutoSize(true);

	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		if (Index > 0)
		{
			USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
			Spacer->SetSize(FVector2D(8.0f, 1.0f));
			ToolbarBox->AddChildToHorizontalBox(Spacer);
		}

		UHorizontalBoxSlot* ButtonSlot = ToolbarBox->AddChildToHorizontalBox(BuildButton(Buttons[Index]));
		ButtonSlot->SetVerticalAlignment(VAlign_Bottom);
	}

	RefreshButtonStates();
}

void UBuildToolbarWidget::SetActiveTool(ESTPBuildTool NewTool)
{
	if (ActiveTool == NewTool)
	{
		return;
	}

	ActiveTool = NewTool;
	RefreshButtonStates();
	BP_ActiveToolChanged(ActiveTool);
}

UWidget* UBuildToolbarWidget::BuildButton(const FBuildToolButtonConfig& Config)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SizeBox->SetWidthOverride(ButtonSize.X);
	SizeBox->SetHeightOverride(ButtonSize.Y);

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetPadding(FMargin(3.0f));
	Border->SetBrushColor(NormalBorderColor);
	SizeBox->AddChild(Border);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetToolTipText(Config.Tooltip);
	Border->SetContent(Button);

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	ApplyIcon(Icon, Config.Tool);
	Button->AddChild(Icon);

	if (Config.Tool == ESTPBuildTool::EnergyCable)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyCableClicked);
	}
	else if (Config.Tool == ESTPBuildTool::EnergyModule)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyModuleClicked);
	}
	else if (Config.Tool == ESTPBuildTool::EnergyStorage)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyStorageClicked);
	}
	else if (Config.Tool == ESTPBuildTool::MiningMachine)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleMiningMachineClicked);
	}

	ButtonBorders.Add(Config.Tool, Border);
	return SizeBox;
}

bool UBuildToolbarWidget::HasDesignedToolbar() const
{
	return EnergyCableButton || EnergyModuleButton || EnergyStorageButton || MiningBuildingButton
		|| EnergyCableIcon || EnergyModuleIcon || EnergyStorageIcon || MiningBuildingIcon
		|| EnergyCableBorder || EnergyModuleBorder || EnergyStorageBorder || MiningBorder;
}

void UBuildToolbarWidget::BindDesignedToolbar()
{
	ButtonBorders.Reset();

	if (EnergyCableButton)
	{
		EnergyCableButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleEnergyCableClicked);
		EnergyCableButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyCableClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::EnergyCable))
		{
			EnergyCableButton->SetToolTipText(Config->Tooltip);
		}
	}

	if (EnergyModuleButton)
	{
		EnergyModuleButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleEnergyModuleClicked);
		EnergyModuleButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyModuleClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::EnergyModule))
		{
			EnergyModuleButton->SetToolTipText(Config->Tooltip);
		}
	}

	if (MiningBuildingButton)
	{
		MiningBuildingButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleMiningMachineClicked);
		MiningBuildingButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleMiningMachineClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::MiningMachine))
		{
			MiningBuildingButton->SetToolTipText(Config->Tooltip);
		}
	}

	if (EnergyStorageButton)
	{
		EnergyStorageButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleEnergyStorageClicked);
		EnergyStorageButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyStorageClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::EnergyStorage))
		{
			EnergyStorageButton->SetToolTipText(Config->Tooltip);
		}
	}

	if (EnergyCableBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::EnergyCable, EnergyCableBorder);
	}

	if (EnergyModuleBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::EnergyModule, EnergyModuleBorder);
	}

	if (MiningBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::MiningMachine, MiningBorder);
	}

	if (EnergyStorageBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::EnergyStorage, EnergyStorageBorder);
	}
}

const FBuildToolButtonConfig* UBuildToolbarWidget::FindButtonConfig(ESTPBuildTool Tool) const
{
	return Buttons.FindByPredicate([Tool](const FBuildToolButtonConfig& Config)
	{
		return Config.Tool == Tool;
	});
}

void UBuildToolbarWidget::ApplyIcon(UImage* Icon, ESTPBuildTool Tool) const
{
	if (!Icon)
	{
		return;
	}

	const FBuildToolButtonConfig* Config = FindButtonConfig(Tool);
	if (Config && Config->IconTexture)
	{
		Icon->SetBrushFromTexture(Config->IconTexture, true);
		Icon->SetColorAndOpacity(FLinearColor::White);
		return;
	}

	FSlateBrush IconBrush;
	IconBrush.ImageSize = FVector2D(FMath::Max(ButtonSize.X - 10.0f, 1.0f), FMath::Max(ButtonSize.Y - 10.0f, 1.0f));
	Icon->SetColorAndOpacity(EmptyIconTint);
	Icon->SetBrush(IconBrush);
}

void UBuildToolbarWidget::HandleEnergyCableClicked()
{
	HandleToolClicked(ESTPBuildTool::EnergyCable);
}

void UBuildToolbarWidget::HandleEnergyModuleClicked()
{
	HandleToolClicked(ESTPBuildTool::EnergyModule);
}

void UBuildToolbarWidget::HandleMiningMachineClicked()
{
	HandleToolClicked(ESTPBuildTool::MiningMachine);
}

void UBuildToolbarWidget::HandleEnergyStorageClicked()
{
	HandleToolClicked(ESTPBuildTool::EnergyStorage);
}

void UBuildToolbarWidget::HandleControllerBuildToolChanged(ESTPBuildTool NewTool)
{
	SetActiveTool(NewTool);
}

void UBuildToolbarWidget::HandleToolClicked(ESTPBuildTool Tool)
{
	const ESTPBuildTool NewTool = ActiveTool == Tool ? ESTPBuildTool::None : Tool;
	SetActiveTool(NewTool);

	if (ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>())
	{
		Controller->SetActiveBuildTool(NewTool);
	}

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Toolbar selected tool=%d"), static_cast<int32>(NewTool));
}

void UBuildToolbarWidget::RefreshButtonStates()
{
	for (const TPair<ESTPBuildTool, TObjectPtr<UBorder>>& ButtonBorder : ButtonBorders)
	{
		if (UBorder* Border = ButtonBorder.Value)
		{
			Border->SetBrushColor(ButtonBorder.Key == ActiveTool ? SelectedBorderColor : NormalBorderColor);
		}
	}
}
