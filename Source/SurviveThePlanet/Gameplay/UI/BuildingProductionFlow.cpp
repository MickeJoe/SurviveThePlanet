#include "Gameplay/UI/BuildingInfoWidget.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"
#include "Gameplay/Buildings/ConcretePlant.h"
#include "Gameplay/Buildings/UraniumProcessor.h"
#include "Gameplay/Buildings/ThermalProcessingBuilding.h"
#include "Gameplay/Buildings/NuclearReactor.h"
#include "Gameplay/Buildings/CrudeWorkshop.h"
#include "Gameplay/Buildings/Steelworks.h"
#include "Gameplay/Buildings/WaterCollector.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/ResourceCatalog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"

namespace
{
	FText FlowAmount(float Amount, bool bPerMinute)
	{
		return FText::FromString(bPerMinute ? FString::Printf(TEXT("%.1f /min"), Amount) : FString::Printf(TEXT("%.0f"), Amount));
	}
}

UUserWidget* UBuildingInfoWidget::AddResourceCard(UHorizontalBox* Box, EResourceType Resource, float Amount, bool bPerMinute)
{
	if (!Box) return nullptr;
	if (Box->GetChildrenCount() > 0)
	{
		UTextBlock* Plus = WidgetTree->ConstructWidget<UTextBlock>();
		Plus->SetText(FText::FromString(TEXT("+")));
		Plus->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.8f, 1.0f)));
		UHorizontalBoxSlot* CardSlot = Box->AddChildToHorizontalBox(Plus);
		CardSlot->SetVerticalAlignment(VAlign_Center);
		CardSlot->SetPadding(FMargin(6.0f));
	}
	UClass* CardClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/BuildingFlow/WBP_FlowResourceCard.WBP_FlowResourceCard_C"));
	UUserWidget* Card = CardClass ? CreateWidget<UUserWidget>(GetOwningPlayer(), CardClass) : nullptr;
	if (!Card) return nullptr;
	Card->SetVisibility(ESlateVisibility::Visible);
	UImage* Icon = Cast<UImage>(Card->GetWidgetFromName(TEXT("ResourceIcon")));
	UTextBlock* Quantity = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("AmountText")));
	if (Icon) Icon->SetBrushFromTexture(UResourceCatalog::GetResourceIcon(Resource));
	if (Quantity) Quantity->SetText(FlowAmount(Amount, bPerMinute));
	for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
	{
		if (Definition.ResourceType == Resource)
		{
			Card->SetToolTipText(Definition.DisplayName);
			if (auto* Name = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("ResourceName")))) Name->SetText(Definition.DisplayName);
			if (Icon) Icon->SetToolTipText(Definition.DisplayName);
			break;
		}
	}
	UHorizontalBoxSlot* CardSlot = Box->AddChildToHorizontalBox(Card);
	CardSlot->SetVerticalAlignment(VAlign_Center);
	return Card;
}

void UBuildingInfoWidget::BuildProductionFlow()
{
	if (!InputsBox || !OutputsBox || !IsValid(Building)) return;
	InputsBox->ClearChildren();
	OutputsBox->ClearChildren();
	DisplayedInputs.Reset();
	InputCards.Reset();
	LiveOutputCard = nullptr;
	DisplayedCycleSeconds = 0.0f;
	const UBuildingDataAsset* Data = Building->GetBuildingData();
	// Legacy catalog entries can use a generic data asset. In that case the
	// production actors use their native recipe defaults, also represented by these CDOs.
	const auto* PolymerRecipe = Cast<UPolymerPlantBuildingDataAsset>(Data);
	if (!PolymerRecipe && Building->GetBuildingType() == ESTPBuildingType::PolymerPlant) PolymerRecipe = GetDefault<UPolymerPlantBuildingDataAsset>();
	const auto* ConnectorRecipe = Cast<UConnectorPlantBuildingDataAsset>(Data);
	if (!ConnectorRecipe && Building->GetBuildingType() == ESTPBuildingType::ConnectorPlant) ConnectorRecipe = GetDefault<UConnectorPlantBuildingDataAsset>();
	const auto* ConcreteRecipe = Cast<UConcretePlantBuildingDataAsset>(Data);
	if (!ConcreteRecipe && Cast<AConcretePlant>(Building)) ConcreteRecipe = GetDefault<UConcretePlantBuildingDataAsset>();
	const auto* SteelRecipe = Cast<USteelworksBuildingDataAsset>(Data);
	if (!SteelRecipe && Cast<ASteelworks>(Building)) SteelRecipe = GetDefault<USteelworksBuildingDataAsset>();
	EResourceType Output = EResourceType::Energy;
	float OutputAmount = Building->GetEnergyProductionPerMinute();
	bool bHasOutput = OutputAmount > 0.0f || Building->GetClass()->GetDefaultObject<ABaseBuilding>()->GetEnergyProductionPerMinute() > 0.0f;
	bool bPerMinute = true;
	if (const auto* Workshop = Cast<ACrudeWorkshop>(Building))
	{
		DisplayedInputs = Workshop->GetRecipeInputs();
		DisplayedCycleSeconds = Workshop->GetCycleSeconds();
		Output = Workshop->GetRecipeOutput(); OutputAmount = 1;
		bHasOutput = true; bPerMinute = false;
	}
	else if (const auto* Reactor = Cast<ANuclearReactor>(Building))
	{
		DisplayedInputs = {{EResourceType::FuelRods, FMath::Max(1, Reactor->FuelRodsPerCycle)},
			{EResourceType::CoolingUnits, FMath::Max(1, Reactor->CoolingUnitsPerCycle)}};
		DisplayedCycleSeconds = FMath::Max(0.1f, Reactor->CycleSeconds);
		Output = EResourceType::Energy; OutputAmount = Reactor->GetEnergyProductionPerMinute();
		bHasOutput = true;
	}
	else if (const auto* Processor = Cast<AUraniumProcessor>(Building))
	{
		DisplayedInputs = {{EResourceType::UraniumOre, FMath::Max(1, Processor->OrePerCycle)}};
		DisplayedCycleSeconds = FMath::Max(0.1f, Processor->CycleSeconds);
		Output = EResourceType::FuelRods; OutputAmount = FMath::Max(1, Processor->FuelRodsPerCycle);
		bHasOutput = true; bPerMinute = false;
	}
	else if (const auto* Thermal = Cast<AThermalProcessingBuilding>(Building))
	{
		DisplayedInputs = Thermal->GetRecipeInputs();
		DisplayedCycleSeconds = FMath::Max(0.1f, Thermal->CycleSeconds);
		Output = Thermal->GetRecipeOutput(); OutputAmount = FMath::Max(1, Thermal->OutputPerCycle);
		bHasOutput = true; bPerMinute = false;
	}
	else if (PolymerRecipe)
	{
		DisplayedInputs = {{EResourceType::Coal, PolymerRecipe->CoalPerCycle}, {EResourceType::Water, PolymerRecipe->WaterPerCycle}};
		DisplayedCycleSeconds = FMath::Max(0.1f, PolymerRecipe->CycleSeconds);
		Output = EResourceType::Polymer; OutputAmount = PolymerRecipe->PolymerPerCycle;
		bHasOutput = true; bPerMinute = false;
	}
	else if (ConnectorRecipe)
	{
		DisplayedInputs = {{EResourceType::Copper, ConnectorRecipe->CopperPerCycle}, {EResourceType::Polymer, ConnectorRecipe->PolymerPerCycle}};
		DisplayedCycleSeconds = FMath::Max(0.1f, ConnectorRecipe->CycleSeconds);
		Output = EResourceType::Connector; OutputAmount = ConnectorRecipe->ConnectorsPerCycle;
		bHasOutput = true; bPerMinute = false;
	}
	else if (ConcreteRecipe)
	{
		DisplayedInputs = {{EResourceType::Water, FMath::CeilToInt(ConcreteRecipe->WaterPerCycle)}, {EResourceType::Stone, FMath::CeilToInt(ConcreteRecipe->StonePerCycle)}};
		DisplayedCycleSeconds = FMath::Max(0.01f, ConcreteRecipe->CycleSeconds);
		Output = EResourceType::Concrete; OutputAmount = FMath::FloorToInt(ConcreteRecipe->ConcretePerCycle);
		bHasOutput = true; bPerMinute = false;
	}
	else if (SteelRecipe)
	{
		DisplayedInputs = {{EResourceType::Iron, FMath::CeilToInt(SteelRecipe->IronPerCycle)}};
		DisplayedCycleSeconds = FMath::Max(0.01f, SteelRecipe->CycleSeconds);
		Output = EResourceType::Steel; OutputAmount = FMath::FloorToInt(SteelRecipe->SteelPerCycle);
		bHasOutput = true; bPerMinute = false;
	}
	else if (const auto* Collector = Cast<AWaterCollector>(Building))
	{
		Output = EResourceType::Water; OutputAmount = Collector->GetCurrentWaterProductionPerMinute();
		bHasOutput = true;
	}
	else if (const auto* Mine = Cast<AMiningMachine>(Building))
	{
		if (const ABaseResourceSource* Source = Mine->GetResourceSource())
		{
			Output = Source->GetResourceType(); OutputAmount = Mine->GetCurrentOutputPerMinute(); bHasOutput = true;
		}
	}
	for (const FResourceCost& Input : DisplayedInputs)
	{
		InputCards.Add(AddResourceCard(InputsBox, Input.Resource, Input.Cost, false));
	}
	auto AddEmpty = [this](UHorizontalBox* Box)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>();
		Empty->SetText(FText::FromString(TEXT("--")));
		Empty->SetToolTipText(FText::FromString(TEXT("No material flow")));
		Box->AddChildToHorizontalBox(Empty)->SetVerticalAlignment(VAlign_Center);
	};
	if (DisplayedInputs.IsEmpty()) AddEmpty(InputsBox);
	if (bHasOutput) LiveOutputCard = AddResourceCard(OutputsBox, Output, OutputAmount, bPerMinute);
	else AddEmpty(OutputsBox);
	if (CycleText)
	{
		CycleText->SetText(DisplayedCycleSeconds > 0 ? FText::FromString(FString::Printf(TEXT("%.0f s"), DisplayedCycleSeconds)) : FText::FromString(TEXT("--")));
		CycleText->SetToolTipText(FText::FromString(DisplayedCycleSeconds > 0 ? TEXT("Recipe amounts per production cycle") : TEXT("Continuous operation; output per minute")));
	}
	if (ConsumptionIcon) ConsumptionIcon->SetBrushFromTexture(UResourceCatalog::GetResourceIcon(EResourceType::Energy));
	if (GenerationIcon) GenerationIcon->SetBrushFromTexture(UResourceCatalog::GetResourceIcon(EResourceType::Energy));
	if (ProductionControls) ProductionControls->SetVisibility(bHasOutput ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (SelectedRecipeText)
	{
		SelectedRecipeText->SetText(FText::GetEmpty());
		for (const FResourceDefinition& Definition : UResourceCatalog::GetDefinitions())
			if (Definition.ResourceType == Output && bHasOutput) SelectedRecipeText->SetText(Definition.DisplayName);
	}
	RefreshProductionFlow();
}

void UBuildingInfoWidget::RefreshProductionFlow()
{
	if (!IsValid(Building)) return;
	if (CycleText && DisplayedCycleSeconds > 0 && (Cast<ANuclearReactor>(Building) || Cast<AUraniumProcessor>(Building) || Cast<AThermalProcessingBuilding>(Building) || Cast<ACrudeWorkshop>(Building)))
	{
		const float Efficiency = Building->GetCombinedDroneEfficiency();
		CycleText->SetText(Efficiency > 0 ? FText::FromString(FString::Printf(TEXT("%.0f s"), DisplayedCycleSeconds / Efficiency)) : FText::FromString(TEXT("--")));
		CycleText->SetToolTipText(FText::FromString(TEXT("Cycle duration at assigned drone efficiency; inputs are consumed together when the cycle completes")));
	}
	const bool bReady = !Building->IsPlacementPreview() && Building->GetConstructionProgress() >= 1.0f;
	const bool bPowered = bReady && Building->CanProduce();
	if (StartProductionButton) StartProductionButton->SetIsEnabled(!Building->IsProductionEnabled());
	if (PauseProductionButton) PauseProductionButton->SetIsEnabled(Building->IsProductionEnabled());
	for (int32 Index = 0; Index < InputCards.Num(); ++Index)
	{
		if (!InputCards[Index]) continue;
		const bool bAvailable = ResourceManager && ResourceManager->HasResource(DisplayedInputs[Index].Resource, DisplayedInputs[Index].Cost);
		if (UWidget* Warning = InputCards[Index]->GetWidgetFromName(TEXT("MissingInputWarning"))) Warning->SetVisibility(bAvailable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (UTextBlock* Amount = Cast<UTextBlock>(InputCards[Index]->GetWidgetFromName(TEXT("AmountText"))))
		{
			const int32 Available = ResourceManager ? ResourceManager->GetResourceAmount(DisplayedInputs[Index].Resource) : 0;
			Amount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Available, DisplayedInputs[Index].Cost)));
			Amount->SetColorAndOpacity(FSlateColor(bAvailable ? FLinearColor::White : FLinearColor(1.0f, 0.3f, 0.18f)));
		}
	}
	if (CycleProgress)
	{
		float Progress = 0.0f;
		if (const auto* Production = Building->FindComponentByClass<UResourceProductionComponent>()) Progress = Production->GetCycleProgressSeconds();
		else if (const auto* Concrete = Cast<AConcretePlant>(Building)) Progress = Concrete->GetCycleProgressSeconds();
		else if (const auto* Steel = Cast<ASteelworks>(Building)) Progress = Steel->GetCycleProgressSeconds();
		CycleProgress->SetVisibility(DisplayedCycleSeconds > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		const float Fraction = DisplayedCycleSeconds > 0 ? FMath::Clamp(Progress / DisplayedCycleSeconds, 0.0f, 1.0f) : 0.0f;
		CycleProgress->SetPercent(Fraction);
		if (CycleProgressText) CycleProgressText->SetText(FText::FromString(FString::Printf(TEXT("%.0f%%"), Fraction * 100.0f)));
	}
	if (LiveOutputCard && Cast<ACrudeWorkshop>(Building))
	{
		if (UTextBlock* Rate = Cast<UTextBlock>(LiveOutputCard->GetWidgetFromName(TEXT("RateText"))))
		{
			const auto* Workshop = CastChecked<ACrudeWorkshop>(Building);
			const float OutputRate = Workshop->IsProducing() ? 60.0f * Workshop->GetCombinedDroneEfficiency() / Workshop->GetCycleSeconds() : 0.0f;
			Rate->SetVisibility(ESlateVisibility::HitTestInvisible);
			Rate->SetText(FText::FromString(FString::Printf(TEXT("+%.1f /min"), OutputRate)));
		}
	}
	if (LiveOutputCard && (DisplayedCycleSeconds <= 0 || Cast<ANuclearReactor>(Building)))
	{
		float Rate = bReady ? Building->GetEnergyProductionPerMinute() : 0.0f;
		if (const auto* Collector = Cast<AWaterCollector>(Building)) Rate = bPowered ? Collector->GetCurrentWaterProductionPerMinute() : 0.0f;
		else if (const auto* Mine = Cast<AMiningMachine>(Building)) Rate = Mine->GetCurrentOutputPerMinute();
		if (UTextBlock* Amount = Cast<UTextBlock>(LiveOutputCard->GetWidgetFromName(TEXT("AmountText")))) Amount->SetText(FlowAmount(Rate, true));
	}
	const float Consumption = Building->GetEnergyConsumptionPerMinute();
	const float Generation = bReady ? Building->GetEnergyProductionPerMinute() : 0.0f;
	if (ConsumptionText)
	{
		ConsumptionText->SetText(FText::FromString(FString::Printf(TEXT("-%.1f /min"), Consumption)));
		ConsumptionText->SetToolTipText(FText::FromString(TEXT("Energy required per minute while operational")));
	}
	if (GenerationText)
	{
		GenerationText->SetText(FText::FromString(FString::Printf(TEXT("+%.1f /min"), Generation)));
		GenerationText->SetToolTipText(FText::FromString(TEXT("Energy generated per minute")));
	}
	const bool bShowConsumption = Consumption > 0 || Generation <= 0;
	if (ConsumptionIcon) ConsumptionIcon->SetVisibility(bShowConsumption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ConsumptionText) ConsumptionText->SetVisibility(bShowConsumption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (GenerationIcon) GenerationIcon->SetVisibility(Generation > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (GenerationText) GenerationText->SetVisibility(Generation > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UBuildingInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!IsValid(Building)) { SetVisibility(ESlateVisibility::Collapsed); return; }
	if (auto* Panel = Cast<USizeBox>(GetWidgetFromName(TEXT("PopupWidth"))))
	{
		const FVector2D View = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(this));
		Panel->SetWidthOverride(FMath::Min(640.0f, View.X - 32.0f));
		Panel->SetMaxDesiredHeight(FMath::Max(240.0f, View.Y - 392.0f));
	}
	FlowRefreshSeconds += InDeltaTime;
	if (FlowRefreshSeconds >= 0.2f)
	{
		FlowRefreshSeconds = 0.0f;
		RefreshPowerStatus();
	}
}
