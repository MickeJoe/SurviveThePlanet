#include "Gameplay/UI/BuildToolbarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "SurviveThePlanet.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"

UBuildToolbarWidget::UBuildToolbarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ButtonSize = FVector2D(72.0f, 72.0f);
	Buttons = {
		{ ESTPBuildTool::EnergyCable, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyCableTooltip", "Energy Cable"), nullptr },
		{ ESTPBuildTool::EnergyModule, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyModuleTooltip", "Energy Module"), nullptr },
		{ ESTPBuildTool::EnergyStorage, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyStorageTooltip", "Build Battery Storage\nIncreases the connected grid's maximum energy capacity."), nullptr },
		{ ESTPBuildTool::MiningMachine, NSLOCTEXT("SurviveThePlanet", "BuildToolMiningMachineTooltip", "Build Mining Machine\nPlace on an available resource deposit."), nullptr },
		{ ESTPBuildTool::WaterCollector, NSLOCTEXT("SurviveThePlanet", "BuildToolWaterCollectorTooltip", "Build Water Collector\nProduces water according to the current rainfall."), nullptr },
		{ ESTPBuildTool::ConcretePlant, NSLOCTEXT("SurviveThePlanet", "BuildToolConcretePlantTooltip", "Build Concrete Plant\nConsumes water, stone and electricity to produce concrete."), nullptr },
		{ ESTPBuildTool::Steelworks, NSLOCTEXT("SurviveThePlanet", "BuildToolSteelworksTooltip", "Build Steelworks\nConsumes iron and electricity to produce steel."), nullptr },
		{ ESTPBuildTool::CommunicationModule, NSLOCTEXT("SurviveThePlanet", "BuildToolCommunicationModuleTooltip", "Build Communication Module\nProvides long-range communications for the colony."), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/CommunicationModuleBuildIcon.CommunicationModuleBuildIcon")) },
		{ ESTPBuildTool::CargoBay, NSLOCTEXT("SurviveThePlanet", "BuildToolCargoBayTooltip", "Build Cargo Bay\nStores cargo and supports colony logistics."), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/CargoBayBuildIcon.CargoBayBuildIcon")) },
		{ ESTPBuildTool::CommandHub, NSLOCTEXT("SurviveThePlanet", "BuildToolCommandHubTooltip", "Build Command Hub"), nullptr },
		{ ESTPBuildTool::SolarArray, NSLOCTEXT("SurviveThePlanet", "BuildToolSolarArrayTooltip", "Build Solar Array"), nullptr },
		{ ESTPBuildTool::WindGenerator, NSLOCTEXT("SurviveThePlanet", "BuildToolWindGeneratorTooltip", "Build Wind Generator"), nullptr },
		{ ESTPBuildTool::GeothermalPlant, NSLOCTEXT("SurviveThePlanet", "BuildToolGeothermalPlantTooltip", "Build Geothermal Plant"), nullptr },
		{ ESTPBuildTool::NuclearReactor, NSLOCTEXT("SurviveThePlanet", "BuildToolNuclearReactorTooltip", "Build Nuclear Reactor"), nullptr },
		{ ESTPBuildTool::MiningStation, NSLOCTEXT("SurviveThePlanet", "BuildToolMiningStationTooltip", "Build Mining Station"), nullptr },
		{ ESTPBuildTool::ResourceStorage, NSLOCTEXT("SurviveThePlanet", "BuildToolResourceStorageTooltip", "Build Resource Storage"), nullptr },
		{ ESTPBuildTool::DroneFactory, NSLOCTEXT("SurviveThePlanet", "BuildToolDroneFactoryTooltip", "Build Drone Factory"), nullptr },
		{ ESTPBuildTool::CommunicationsTower, NSLOCTEXT("SurviveThePlanet", "BuildToolCommunicationsTowerTooltip", "Build Communications Tower"), nullptr }
	};
}

TSharedRef<SWidget> UBuildToolbarWidget::RebuildWidget()
{
	EnsureRequiredButtonConfigs();
	RefreshButtonConfigsFromCatalog();
	RebuildToolbar();
	return Super::RebuildWidget();
}

void UBuildToolbarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureRequiredButtonConfigs();
	RefreshButtonConfigsFromCatalog();

	if (ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>())
	{
		Controller->OnBuildToolChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
		Controller->OnBuildToolChanged.AddDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
		SetActiveTool(Controller->GetActiveBuildTool());
	}
	if (UGameInstance* GI = GetGameInstance()) if (UBuildingBlueprintSubsystem* Inventory = GI->GetSubsystem<UBuildingBlueprintSubsystem>())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleBlueprintInventoryChanged);
		Inventory->OnInventoryChanged.AddDynamic(this, &UBuildToolbarWidget::HandleBlueprintInventoryChanged);
	}
	ResourceManager = ResolveResourceManager();
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleResourceAmountChanged);
		ResourceManager->OnResourceAmountChanged.AddDynamic(this, &UBuildToolbarWidget::HandleResourceAmountChanged);
	}
	RefreshButtonStates();
}

void UBuildToolbarWidget::EnsureRequiredButtonConfigs()
{
	const FBuildToolButtonConfig Required[] =
	{
		{ ESTPBuildTool::EnergyCable, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyCableTooltip", "Energy Cable"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/ConstructCableConnectionIcon.ConstructCableConnectionIcon")) },
		{ ESTPBuildTool::EnergyModule, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyModuleTooltip", "Energy Module"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/ConstructEnergryBuildingIcon.ConstructEnergryBuildingIcon")) },
		{ ESTPBuildTool::EnergyStorage, NSLOCTEXT("SurviveThePlanet", "BuildToolEnergyStorageTooltip", "Build Battery Storage"), nullptr },
		{ ESTPBuildTool::MiningMachine, NSLOCTEXT("SurviveThePlanet", "BuildToolMiningMachineTooltip", "Build Mining Machine"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/ConstructMiningBuildingIcon.ConstructMiningBuildingIcon")) },
		{ ESTPBuildTool::WaterCollector, NSLOCTEXT("SurviveThePlanet", "BuildToolWaterCollectorTooltip", "Build Water Collector"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/WaterCollectorBuildIcon.WaterCollectorBuildIcon")) },
		{ ESTPBuildTool::ConcretePlant, NSLOCTEXT("SurviveThePlanet", "BuildToolConcretePlantTooltip", "Build Concrete Plant"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/ConcretePlantBuildIcon.ConcretePlantBuildIcon")) },
		{ ESTPBuildTool::Steelworks, NSLOCTEXT("SurviveThePlanet", "BuildToolSteelworksTooltip", "Build Steelworks"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Icons/Buildings/T_Steelworks.T_Steelworks")) },
		{ ESTPBuildTool::CommunicationModule, NSLOCTEXT("SurviveThePlanet", "BuildToolCommunicationModuleTooltip", "Build Communication Module"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/CommunicationModuleBuildIcon.CommunicationModuleBuildIcon")) },
		{ ESTPBuildTool::CargoBay, NSLOCTEXT("SurviveThePlanet", "BuildToolCargoBayTooltip", "Build Cargo Bay"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Images/CargoBayBuildIcon.CargoBayBuildIcon")) },
		{ ESTPBuildTool::CommandHub, NSLOCTEXT("SurviveThePlanet", "BuildToolCommandHubTooltip", "Build Command Hub"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_CommandHub_preview.STP_CommandHub_preview")) },
		{ ESTPBuildTool::SolarArray, NSLOCTEXT("SurviveThePlanet", "BuildToolSolarArrayTooltip", "Build Solar Array"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_SolarArray_preview.STP_SolarArray_preview")) },
		{ ESTPBuildTool::WindGenerator, NSLOCTEXT("SurviveThePlanet", "BuildToolWindGeneratorTooltip", "Build Wind Generator"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_WindGenerator_preview.STP_WindGenerator_preview")) },
		{ ESTPBuildTool::GeothermalPlant, NSLOCTEXT("SurviveThePlanet", "BuildToolGeothermalPlantTooltip", "Build Geothermal Plant"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_GeothermalPlant_preview.STP_GeothermalPlant_preview")) },
		{ ESTPBuildTool::NuclearReactor, NSLOCTEXT("SurviveThePlanet", "BuildToolNuclearReactorTooltip", "Build Nuclear Reactor"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_NuclearReactor_preview.STP_NuclearReactor_preview")) },
		{ ESTPBuildTool::MiningStation, NSLOCTEXT("SurviveThePlanet", "BuildToolMiningStationTooltip", "Build Mining Station"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_MiningStation_preview.STP_MiningStation_preview")) },
		{ ESTPBuildTool::ResourceStorage, NSLOCTEXT("SurviveThePlanet", "BuildToolResourceStorageTooltip", "Build Resource Storage"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_ResourceStorage_preview.STP_ResourceStorage_preview")) },
		{ ESTPBuildTool::DroneFactory, NSLOCTEXT("SurviveThePlanet", "BuildToolDroneFactoryTooltip", "Build Drone Factory"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_DroneFactory_preview.STP_DroneFactory_preview")) },
		{ ESTPBuildTool::CommunicationsTower, NSLOCTEXT("SurviveThePlanet", "BuildToolCommunicationsTowerTooltip", "Build Communications Tower"), LoadObject<UTexture2D>(nullptr, TEXT("/Game/Units/Buildings/STP/STP_CommunicationsTower_preview.STP_CommunicationsTower_preview")) }
	};

	for (const FBuildToolButtonConfig& RequiredConfig : Required)
	{
		if (!FindButtonConfig(RequiredConfig.Tool))
		{
			Buttons.Add(RequiredConfig);
		}
	}
}

void UBuildToolbarWidget::RefreshButtonConfigsFromCatalog()
{
	UWorld* World = GetWorld();
	UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	if (!Manager) return;

	for (UBuildingDataAsset* Definition : Manager->GetToolbarDefinitions())
	{
		if (!Definition || Definition->BuildTool == ESTPBuildTool::None) continue;
		FBuildToolButtonConfig* Config = Buttons.FindByPredicate([Definition](const FBuildToolButtonConfig& Entry)
		{
			return Entry.Tool == Definition->BuildTool;
		});
		if (!Config)
		{
			Config = &Buttons.AddDefaulted_GetRef();
			Config->Tool = Definition->BuildTool;
		}
		Config->Tooltip = FText::Format(NSLOCTEXT("SurviveThePlanet", "CatalogBuildTooltip", "Build {0}\n{1}"),
			Definition->DisplayName, Definition->Description);
		Config->IconTexture = Definition->ToolbarIcon ? Definition->ToolbarIcon : Definition->Thumbnail;
	}
}

void UBuildToolbarWidget::NativeDestruct()
{
	if (ASurviveThePlanetPlayerController* Controller = GetOwningPlayer<ASurviveThePlanetPlayerController>())
	{
		Controller->OnBuildToolChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleControllerBuildToolChanged);
	}
	if (UGameInstance* GI = GetGameInstance()) if (UBuildingBlueprintSubsystem* Inventory = GI->GetSubsystem<UBuildingBlueprintSubsystem>())
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleBlueprintInventoryChanged);
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(this, &UBuildToolbarWidget::HandleResourceAmountChanged);
		ResourceManager = nullptr;
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
	ToolButtons.Reset();
	ToolWidgets.Reset();
	CategoryRows.Reset();
	CategoryBorders.Reset();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildToolbarRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UVerticalBox* ToolbarStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildToolbarStack"));
	UCanvasPanelSlot* ToolbarSlot = RootCanvas->AddChildToCanvas(ToolbarStack);
	ToolbarSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	ToolbarSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	ToolbarSlot->SetPosition(FVector2D(0.0f, -24.0f));
	ToolbarSlot->SetAutoSize(true);

	TMap<ESTPBuildCategory,UHorizontalBox*> Rows;
	for (ESTPBuildCategory Category : {ESTPBuildCategory::Energy, ESTPBuildCategory::Industry, ESTPBuildCategory::Logistics, ESTPBuildCategory::Infrastructure})
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBoxSlot* RowSlot = ToolbarStack->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		Rows.Add(Category, Row);
		CategoryRows.Add(Category, Row);
	}
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UHorizontalBox* ToolBox=Rows.FindRef(GetCategoryForTool(Buttons[Index].Tool)); if(!ToolBox)continue;
		UWidget* ToolWidget=BuildButton(Buttons[Index]); ToolWidgets.Add(Buttons[Index].Tool,ToolWidget);
		UHorizontalBoxSlot* ButtonSlot = ToolBox->AddChildToHorizontalBox(ToolWidget);
		ButtonSlot->SetPadding(FMargin(0,0,8,0));
		ButtonSlot->SetVerticalAlignment(VAlign_Bottom);
	}
	UHorizontalBox* Categories = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildCategories"));
	UVerticalBoxSlot* CategoriesSlot = ToolbarStack->AddChildToVerticalBox(Categories);
	CategoriesSlot->SetHorizontalAlignment(HAlign_Center);
	auto AddCategory = [this, Categories](const FText& Label, ESTPBuildCategory Category)
	{
		UHorizontalBoxSlot* Slot = Categories->AddChildToHorizontalBox(BuildCategoryButton(Label, Category));
		Slot->SetPadding(FMargin(4.0f, 0.0f));
		Slot->SetVerticalAlignment(VAlign_Bottom);
	};
	AddCategory(NSLOCTEXT("STPBuild", "EnergyCategory", "ENERGY"), ESTPBuildCategory::Energy);
	AddCategory(NSLOCTEXT("STPBuild", "IndustryCategory", "INDUSTRY"), ESTPBuildCategory::Industry);
	AddCategory(NSLOCTEXT("STPBuild", "LogisticsCategory", "LOGISTICS"), ESTPBuildCategory::Logistics);
	AddCategory(NSLOCTEXT("STPBuild", "InfrastructureCategory", "INFRASTRUCTURE"), ESTPBuildCategory::Infrastructure);

	RefreshToolbarVisibility();
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
	Border->SetPadding(FMargin(0.0f));
	Border->SetBrushColor(NormalBorderColor);
	SizeBox->AddChild(Border);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetToolTipText(Config.Tooltip);
	FButtonStyle ButtonStyle = Button->GetStyle();
	ButtonStyle.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
	ButtonStyle.Hovered.DrawAs = ESlateBrushDrawType::RoundedBox;
	ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.0f, 0.72f, 0.95f, 0.18f));
	ButtonStyle.Pressed.DrawAs = ESlateBrushDrawType::RoundedBox;
	ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.0f, 0.72f, 0.95f, 0.30f));
	ButtonStyle.Disabled.DrawAs = ESlateBrushDrawType::RoundedBox;
	ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(0.03f, 0.045f, 0.055f, 0.72f));
	ButtonStyle.NormalPadding = FMargin(0.0f);
	ButtonStyle.PressedPadding = FMargin(0.0f);
	Button->SetStyle(ButtonStyle);
	Button->SetBackgroundColor(FLinearColor::White);
	Border->SetContent(Button);

	UOverlay* ButtonContent = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Button->AddChild(ButtonContent);

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	ApplyIcon(Icon, Config.Tool);
	Icon->SetDesiredSizeOverride(ButtonSize);
	UOverlaySlot* IconSlot = ButtonContent->AddChildToOverlay(Icon);
	IconSlot->SetHorizontalAlignment(HAlign_Fill);
	IconSlot->SetVerticalAlignment(VAlign_Fill);
	IconSlot->SetPadding(FMargin(0.0f));

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
	else if (Config.Tool == ESTPBuildTool::WaterCollector)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleWaterCollectorClicked);
	}
	else if (Config.Tool == ESTPBuildTool::ConcretePlant)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleConcretePlantClicked);
	}
	else if (Config.Tool == ESTPBuildTool::Steelworks)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleSteelworksClicked);
	}
	else if (Config.Tool == ESTPBuildTool::CommunicationModule)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCommunicationModuleClicked);
	}
	else if (Config.Tool == ESTPBuildTool::CargoBay)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCargoBayClicked);
	}
	else if (Config.Tool == ESTPBuildTool::CommandHub) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCommandHubClicked);
	else if (Config.Tool == ESTPBuildTool::SolarArray) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleSolarArrayClicked);
	else if (Config.Tool == ESTPBuildTool::WindGenerator) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleWindGeneratorClicked);
	else if (Config.Tool == ESTPBuildTool::GeothermalPlant) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleGeothermalPlantClicked);
	else if (Config.Tool == ESTPBuildTool::NuclearReactor) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleNuclearReactorClicked);
	else if (Config.Tool == ESTPBuildTool::MiningStation) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleMiningStationClicked);
	else if (Config.Tool == ESTPBuildTool::ResourceStorage) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleResourceStorageClicked);
	else if (Config.Tool == ESTPBuildTool::DroneFactory) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleDroneFactoryClicked);
	else if (Config.Tool == ESTPBuildTool::CommunicationsTower) Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCommunicationsTowerClicked);

	ButtonBorders.Add(Config.Tool, Border);
	ToolButtons.Add(Config.Tool, Button);
	return SizeBox;
}

bool UBuildToolbarWidget::HasDesignedToolbar() const
{
	return EnergyCableButton || EnergyModuleButton || EnergyStorageButton || MiningBuildingButton || WaterCollectorButton || ConcretePlantButton || CommunicationModuleButton || CargoBayButton
		|| EnergyCableIcon || EnergyModuleIcon || EnergyStorageIcon || MiningBuildingIcon || WaterCollectorIcon || ConcretePlantIcon
		|| EnergyCableBorder || EnergyModuleBorder || EnergyStorageBorder || MiningBorder || WaterCollectorBorder || ConcretePlantBorder;
}

void UBuildToolbarWidget::BindDesignedToolbar()
{
	ButtonBorders.Reset();
	ToolButtons.Reset();

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

	if (WaterCollectorButton)
	{
		WaterCollectorButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleWaterCollectorClicked);
		WaterCollectorButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleWaterCollectorClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::WaterCollector))
		{
			WaterCollectorButton->SetToolTipText(Config->Tooltip);
		}
	}

	if (ConcretePlantButton)
	{
		ConcretePlantButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleConcretePlantClicked);
		ConcretePlantButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleConcretePlantClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::ConcretePlant)) ConcretePlantButton->SetToolTipText(Config->Tooltip);
	}

	if (CommunicationModuleButton)
	{
		CommunicationModuleButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleCommunicationModuleClicked);
		CommunicationModuleButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCommunicationModuleClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::CommunicationModule)) CommunicationModuleButton->SetToolTipText(Config->Tooltip);
	}

	if (CargoBayButton)
	{
		CargoBayButton->OnClicked.RemoveDynamic(this, &UBuildToolbarWidget::HandleCargoBayClicked);
		CargoBayButton->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleCargoBayClicked);
		if (const FBuildToolButtonConfig* Config = FindButtonConfig(ESTPBuildTool::CargoBay)) CargoBayButton->SetToolTipText(Config->Tooltip);
	}

	ApplyIcon(EnergyCableIcon, ESTPBuildTool::EnergyCable);
	ApplyIcon(EnergyModuleIcon, ESTPBuildTool::EnergyModule);
	ApplyIcon(EnergyStorageIcon, ESTPBuildTool::EnergyStorage);
	ApplyIcon(MiningBuildingIcon, ESTPBuildTool::MiningMachine);
	ApplyIcon(WaterCollectorIcon, ESTPBuildTool::WaterCollector);
	ApplyIcon(ConcretePlantIcon, ESTPBuildTool::ConcretePlant);
	ApplyIcon(CargoBayIcon, ESTPBuildTool::CargoBay);
	// The Communication Module artwork is authored directly in WBP_BuildToolbar.
	// Preserve that brush instead of replacing it during NativeConstruct.

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

	if (WaterCollectorBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::WaterCollector, WaterCollectorBorder);
	}

	if (ConcretePlantBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::ConcretePlant, ConcretePlantBorder);
	}

	if (CommunicationModuleBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::CommunicationModule, CommunicationModuleBorder);
	}
	if (CargoBayBorder)
	{
		ButtonBorders.Add(ESTPBuildTool::CargoBay, CargoBayBorder);
	}

	if (EnergyCableButton) ToolButtons.Add(ESTPBuildTool::EnergyCable, EnergyCableButton);
	if (EnergyModuleButton) ToolButtons.Add(ESTPBuildTool::EnergyModule, EnergyModuleButton);
	if (EnergyStorageButton) ToolButtons.Add(ESTPBuildTool::EnergyStorage, EnergyStorageButton);
	if (MiningBuildingButton) ToolButtons.Add(ESTPBuildTool::MiningMachine, MiningBuildingButton);
	if (WaterCollectorButton) ToolButtons.Add(ESTPBuildTool::WaterCollector, WaterCollectorButton);
	if (ConcretePlantButton) ToolButtons.Add(ESTPBuildTool::ConcretePlant, ConcretePlantButton);
	if (CommunicationModuleButton) ToolButtons.Add(ESTPBuildTool::CommunicationModule, CommunicationModuleButton);
	if (CargoBayButton) ToolButtons.Add(ESTPBuildTool::CargoBay, CargoBayButton);
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

	// Preserve artwork authored directly in the designed WBP. This also keeps the
	// Cargo Bay icon visible while catalog data is being reloaded in the editor.
	if (Icon->GetBrush().GetResourceObject())
	{
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

void UBuildToolbarWidget::HandleWaterCollectorClicked()
{
	HandleToolClicked(ESTPBuildTool::WaterCollector);
}

void UBuildToolbarWidget::HandleConcretePlantClicked()
{
	HandleToolClicked(ESTPBuildTool::ConcretePlant);
}

void UBuildToolbarWidget::HandleSteelworksClicked()
{
	HandleToolClicked(ESTPBuildTool::Steelworks);
}

void UBuildToolbarWidget::HandleCommunicationModuleClicked()
{
	HandleToolClicked(ESTPBuildTool::CommunicationModule);
}

void UBuildToolbarWidget::HandleCargoBayClicked()
{
	HandleToolClicked(ESTPBuildTool::CargoBay);
}

void UBuildToolbarWidget::HandleControllerBuildToolChanged(ESTPBuildTool NewTool)
{
	SetActiveTool(NewTool);
}

void UBuildToolbarWidget::HandleToolClicked(ESTPBuildTool Tool)
{
	TArray<FResourceCost> Costs;
	if (!IsToolOwned(Tool) || !CanAffordTool(Tool, &Costs))
	{
		RefreshButtonStates();
		return;
	}

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
	if (!ResourceManager)
	{
		ResourceManager = ResolveResourceManager();
	}

	for (const TPair<ESTPBuildTool, TObjectPtr<UButton>>& Entry : ToolButtons)
	{
		if (!Entry.Value)
		{
			continue;
		}

		TArray<FResourceCost> Costs;
		const bool bOwned = IsToolOwned(Entry.Key);
		const bool bAffordable = CanAffordTool(Entry.Key, &Costs);
		Entry.Value->SetIsEnabled(bOwned && bAffordable);
		const FText Tooltip = BuildToolTooltip(Entry.Key, bAffordable, Costs);
		Entry.Value->SetToolTipText(Tooltip);
		if (const TObjectPtr<UBorder>* Border = ButtonBorders.Find(Entry.Key); Border && *Border)
		{
			(*Border)->SetToolTipText(Tooltip);
		}
	}

	for (const TPair<ESTPBuildTool, TObjectPtr<UBorder>>& ButtonBorder : ButtonBorders)
	{
		if (UBorder* Border = ButtonBorder.Value)
		{
			const TObjectPtr<UButton>* Button = ToolButtons.Find(ButtonBorder.Key);
			const bool bEnabled = !Button || !*Button || (*Button)->GetIsEnabled();
			Border->SetBrushColor(!bEnabled
				? FLinearColor(NormalBorderColor.R * 0.45f, NormalBorderColor.G * 0.45f, NormalBorderColor.B * 0.45f, NormalBorderColor.A)
				: (ButtonBorder.Key == ActiveTool ? SelectedBorderColor : NormalBorderColor));
		}
	}
}

bool UBuildToolbarWidget::CanAffordTool(ESTPBuildTool Tool, TArray<FResourceCost>* OutCosts) const
{
	TArray<FResourceCost> Costs;
	if (const UWorld* World = GetWorld())
	{
		if (const UBuildingManagerSubsystem* Manager = World->GetSubsystem<UBuildingManagerSubsystem>())
		{
			if (const UBuildingDataAsset* Definition = Manager->GetDefinition(Tool))
			{
				Costs = Definition->ConstructionCosts;
			}
		}
	}

	if (OutCosts)
	{
		*OutCosts = Costs;
	}
	return Costs.IsEmpty() || (ResourceManager && ResourceManager->CanAffordCosts(Costs));
}

FText UBuildToolbarWidget::BuildToolTooltip(ESTPBuildTool Tool, bool bAffordable, const TArray<FResourceCost>& Costs) const
{
	const FBuildToolButtonConfig* Config = FindButtonConfig(Tool);
	const FText BaseTooltip = Config ? Config->Tooltip : FText::GetEmpty();
	if (bAffordable || Costs.IsEmpty())
	{
		return BaseTooltip;
	}

	TMap<EResourceType, int32> CombinedCosts;
	for (const FResourceCost& Cost : Costs)
	{
		CombinedCosts.FindOrAdd(Cost.Resource) += FMath::Max(Cost.Cost, 0);
	}

	TArray<FString> MissingLines;
	for (const TPair<EResourceType, int32>& Cost : CombinedCosts)
	{
		const int32 Available = ResourceManager ? ResourceManager->GetResourceAmount(Cost.Key) : 0;
		if (Available < Cost.Value)
		{
			const FText ResourceName = StaticEnum<EResourceType>()->GetDisplayNameTextByValue(static_cast<int64>(Cost.Key));
			MissingLines.Add(FString::Printf(TEXT("%s: %d / %d"), *ResourceName.ToString(), Available, Cost.Value));
		}
	}

	const FText MissingText = FText::FromString(FString::Join(MissingLines, TEXT("\n")));
	return FText::Format(
		NSLOCTEXT("SurviveThePlanet", "BuildToolInsufficientResources", "{0}\n\nNot enough resources:\n{1}"),
		BaseTooltip,
		MissingText);
}

AResourceManager* UBuildToolbarWidget::ResolveResourceManager()
{
	if (ResourceManager)
	{
		return ResourceManager;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

bool UBuildToolbarWidget::IsToolOwned(ESTPBuildTool Tool) const
{
	const UGameInstance* GI = GetGameInstance(); const UBuildingBlueprintSubsystem* Inventory = GI ? GI->GetSubsystem<UBuildingBlueprintSubsystem>() : nullptr;
	return !Inventory || Inventory->OwnsBlueprint(Tool);
}

ESTPBuildCategory UBuildToolbarWidget::GetCategoryForTool(ESTPBuildTool Tool) const
{
	switch (Tool)
	{
	case ESTPBuildTool::EnergyCable:
	case ESTPBuildTool::EnergyModule:
	case ESTPBuildTool::EnergyStorage:
	case ESTPBuildTool::SolarArray:
	case ESTPBuildTool::WindGenerator:
	case ESTPBuildTool::GeothermalPlant:
	case ESTPBuildTool::NuclearReactor:
		return ESTPBuildCategory::Energy;
	case ESTPBuildTool::MiningMachine:
	case ESTPBuildTool::MiningStation:
	case ESTPBuildTool::ConcretePlant:
	case ESTPBuildTool::DroneFactory:
	case ESTPBuildTool::Steelworks:
		return ESTPBuildCategory::Industry;
	case ESTPBuildTool::CargoBay:
	case ESTPBuildTool::ResourceStorage:
		return ESTPBuildCategory::Logistics;
	case ESTPBuildTool::CommandHub:
	case ESTPBuildTool::WaterCollector:
	case ESTPBuildTool::CommunicationModule:
	case ESTPBuildTool::CommunicationsTower:
		return ESTPBuildCategory::Infrastructure;
	default:
		break;
	}

	UWorld* World = GetWorld();
	UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	if (const UBuildingDataAsset* Definition = Manager ? Manager->GetDefinition(Tool) : nullptr)
	{
		return Definition->BuildCategory;
	}
	return ESTPBuildCategory::Infrastructure;
}

UWidget* UBuildToolbarWidget::BuildCategoryButton(const FText& Label, ESTPBuildCategory Category)
{
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SizeBox->SetWidthOverride(CategoryButtonSize.X);
	SizeBox->SetHeightOverride(CategoryButtonSize.Y);

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Border->SetPadding(FMargin(2.0f));
	Border->SetBrushColor(CategoryNormalColor);
	SizeBox->AddChild(Border);
	CategoryBorders.Add(Category, Border);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetToolTipText(Label);
	Button->SetBackgroundColor(CategoryButtonBackground);
	Button->SetColorAndOpacity(FLinearColor::White);
	Border->SetContent(Button);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Button->SetContent(Content);

	UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Texture = GetCategoryIcon(Category))
	{
		Icon->SetBrushFromTexture(Texture, true);
	}
	UVerticalBoxSlot* IconSlot = Content->AddChildToVerticalBox(Icon);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Category == ESTPBuildCategory::Infrastructure ? 11 : 13;
	Text->SetFont(Font);
	UVerticalBoxSlot* TextSlot = Content->AddChildToVerticalBox(Text);
	TextSlot->SetHorizontalAlignment(HAlign_Fill);
	TextSlot->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 3.0f));

	if (Category == ESTPBuildCategory::Energy)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleEnergyCategoryClicked);
	}
	else if (Category == ESTPBuildCategory::Industry)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleIndustryCategoryClicked);
	}
	else if (Category == ESTPBuildCategory::Logistics)
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleLogisticsCategoryClicked);
	}
	else
	{
		Button->OnClicked.AddDynamic(this, &UBuildToolbarWidget::HandleInfrastructureCategoryClicked);
	}

	return SizeBox;
}

UTexture2D* UBuildToolbarWidget::GetCategoryIcon(ESTPBuildCategory Category) const
{
	const TCHAR* Path = nullptr;
	switch (Category)
	{
	case ESTPBuildCategory::Energy:
		Path = TEXT("/Game/UI/Images/BuildCategories/T_Category_Energy.T_Category_Energy");
		break;
	case ESTPBuildCategory::Industry:
		Path = TEXT("/Game/UI/Images/BuildCategories/T_Category_Industry.T_Category_Industry");
		break;
	case ESTPBuildCategory::Logistics:
		Path = TEXT("/Game/UI/Images/BuildCategories/T_Category_Logistics.T_Category_Logistics");
		break;
	case ESTPBuildCategory::Infrastructure:
		Path = TEXT("/Game/UI/Images/BuildCategories/T_Category_Infrastructure.T_Category_Infrastructure");
		break;
	default:
		break;
	}
	return Path ? LoadObject<UTexture2D>(nullptr, Path) : nullptr;
}
void UBuildToolbarWidget::HandleEnergyCategoryClicked(){ActiveCategory=ESTPBuildCategory::Energy;RefreshToolbarVisibility();}
void UBuildToolbarWidget::HandleIndustryCategoryClicked(){ActiveCategory=ESTPBuildCategory::Industry;RefreshToolbarVisibility();}
void UBuildToolbarWidget::HandleLogisticsCategoryClicked(){ActiveCategory=ESTPBuildCategory::Logistics;RefreshToolbarVisibility();}
void UBuildToolbarWidget::HandleInfrastructureCategoryClicked(){ActiveCategory=ESTPBuildCategory::Infrastructure;RefreshToolbarVisibility();}
void UBuildToolbarWidget::HandleCommandHubClicked(){HandleToolClicked(ESTPBuildTool::CommandHub);} void UBuildToolbarWidget::HandleSolarArrayClicked(){HandleToolClicked(ESTPBuildTool::SolarArray);} void UBuildToolbarWidget::HandleWindGeneratorClicked(){HandleToolClicked(ESTPBuildTool::WindGenerator);} void UBuildToolbarWidget::HandleGeothermalPlantClicked(){HandleToolClicked(ESTPBuildTool::GeothermalPlant);} void UBuildToolbarWidget::HandleNuclearReactorClicked(){HandleToolClicked(ESTPBuildTool::NuclearReactor);} void UBuildToolbarWidget::HandleMiningStationClicked(){HandleToolClicked(ESTPBuildTool::MiningStation);} void UBuildToolbarWidget::HandleResourceStorageClicked(){HandleToolClicked(ESTPBuildTool::ResourceStorage);} void UBuildToolbarWidget::HandleDroneFactoryClicked(){HandleToolClicked(ESTPBuildTool::DroneFactory);} void UBuildToolbarWidget::HandleCommunicationsTowerClicked(){HandleToolClicked(ESTPBuildTool::CommunicationsTower);}
void UBuildToolbarWidget::HandleBlueprintInventoryChanged(ESTPBuildTool ChangedTool){RefreshToolbarVisibility();}

void UBuildToolbarWidget::HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount)
{
	RefreshButtonStates();
}

void UBuildToolbarWidget::RefreshToolbarVisibility()
{
	for(const TPair<ESTPBuildCategory,TObjectPtr<UWidget>>& Pair:CategoryRows) if(Pair.Value) Pair.Value->SetVisibility(Pair.Key==ActiveCategory?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
	for(const TPair<ESTPBuildTool,TObjectPtr<UWidget>>& Pair:ToolWidgets) if(Pair.Value) Pair.Value->SetVisibility(IsToolOwned(Pair.Key)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
	for (const TPair<ESTPBuildCategory, TObjectPtr<UBorder>>& Pair : CategoryBorders)
	{
		if (Pair.Value)
		{
			Pair.Value->SetBrushColor(Pair.Key == ActiveCategory ? CategorySelectedColor : CategoryNormalColor);
		}
	}
}
