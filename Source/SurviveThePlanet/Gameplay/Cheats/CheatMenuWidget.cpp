#include "Gameplay/Cheats/CheatMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Gameplay/Cheats/STPCheatManager.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "GameFramework/PlayerController.h"

void UCheatMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
#if !UE_BUILD_SHIPPING
	if (!ResourceComboBox || !AmountSpinBox || !GiveButton)
	{
		BuildFallbackLayout();
	}
	PopulateResources();
	AmountSpinBox->SetMinValue(1.0f);
	AmountSpinBox->SetMaxValue(1000000.0f);
	AmountSpinBox->SetMinSliderValue(1.0f);
	AmountSpinBox->SetMaxSliderValue(10000.0f);
	AmountSpinBox->SetValue(100.0f);
	GiveButton->OnClicked.AddUniqueDynamic(this, &UCheatMenuWidget::GiveSelectedResource);
	EnsureObjectiveCheatButton();
	CompleteObjectiveButton->OnClicked.AddUniqueDynamic(this, &UCheatMenuWidget::CompleteUplinkObjective);
	EnsureBlueprintCheatControls();
	GrantBlueprintButton->OnClicked.AddUniqueDynamic(this, &UCheatMenuWidget::GrantSelectedBlueprint);
#endif
}

void UCheatMenuWidget::EnsureBlueprintCheatControls()
{
	TArray<UWidget*> Widgets; WidgetTree->GetAllWidgets(Widgets);
	UVerticalBox* Column = nullptr;
	for (UWidget* Widget : Widgets) if ((Column = Cast<UVerticalBox>(Widget))) break;
	if (!Column) return;
	if (!BlueprintComboBox)
	{
		BlueprintComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("BlueprintComboBox"));
		Column->AddChildToVerticalBox(BlueprintComboBox)->SetPadding(FMargin(0, 10, 0, 4));
	}
	if (!GrantBlueprintButton)
	{
		GrantBlueprintButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GrantBlueprintButton"));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GrantBlueprintButtonLabel"));
		Label->SetText(NSLOCTEXT("STPCheats", "GrantBlueprint", "OWN BUILDING BLUEPRINT"));
		GrantBlueprintButton->SetContent(Label); Column->AddChildToVerticalBox(GrantBlueprintButton);
	}
	BlueprintComboBox->ClearOptions(); BlueprintTools.Reset();
	UWorld* World = GetWorld(); UBuildingManagerSubsystem* Manager = World ? World->GetSubsystem<UBuildingManagerSubsystem>() : nullptr;
	if (Manager)
	{
		for (UBuildingDataAsset* Definition : Manager->GetAllDefinitions())
		{
			if (!Definition || Definition->BuildTool == ESTPBuildTool::None) continue;
			BlueprintTools.Add(Definition->BuildTool); BlueprintComboBox->AddOption(Definition->DisplayName.ToString());
		}
	}
	if (BlueprintComboBox->GetOptionCount() > 0) BlueprintComboBox->SetSelectedIndex(0);
}

void UCheatMenuWidget::GrantSelectedBlueprint()
{
#if !UE_BUILD_SHIPPING
	const int32 Index = BlueprintComboBox ? BlueprintComboBox->GetSelectedIndex() : INDEX_NONE;
	APlayerController* PC = GetOwningPlayer(); USTPCheatManager* Cheats = PC ? Cast<USTPCheatManager>(PC->CheatManager) : nullptr;
	const bool bSuccess = BlueprintTools.IsValidIndex(Index) && Cheats && Cheats->GrantBuildingBlueprint(BlueprintTools[Index]);
	if (FeedbackText) FeedbackText->SetText(bSuccess ? NSLOCTEXT("STPCheats", "BlueprintGranted", "Building blueprint acquired; build menu updated.") : NSLOCTEXT("STPCheats", "BlueprintOwned", "Blueprint already owned or unavailable."));
#endif
}

void UCheatMenuWidget::EnsureObjectiveCheatButton()
{
	if (CompleteObjectiveButton)
	{
		return;
	}
	CompleteObjectiveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CompleteObjectiveButton"));
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CompleteObjectiveButtonLabel"));
	Label->SetText(NSLOCTEXT("STPCheats", "CompleteUplink", "Complete ESTABLISH UPLINK"));
	CompleteObjectiveButton->SetContent(Label);

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (UVerticalBox* Column = Cast<UVerticalBox>(Widget))
		{
			Column->AddChildToVerticalBox(CompleteObjectiveButton)->SetPadding(FMargin(0, 8, 0, 0));
			return;
		}
	}
}

void UCheatMenuWidget::BuildFallbackLayout()
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CheatPanel"));
	Border->SetPadding(FMargin(20.0f));
	Border->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.06f, 0.96f));
	WidgetTree->RootWidget = Border;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Border->SetContent(Column);
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(NSLOCTEXT("STPCheats", "Title", "CHEAT MENU"));
	Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 12));

	ResourceComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ResourceComboBox"));
	Column->AddChildToVerticalBox(ResourceComboBox)->SetPadding(FMargin(0, 0, 0, 8));
	AmountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("AmountSpinBox"));
	Column->AddChildToVerticalBox(AmountSpinBox)->SetPadding(FMargin(0, 0, 0, 8));
	GiveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GiveButton"));
	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GiveButtonLabel"));
	ButtonLabel->SetText(NSLOCTEXT("STPCheats", "Give", "Give resource"));
	GiveButton->SetContent(ButtonLabel);
	Column->AddChildToVerticalBox(GiveButton);
	FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FeedbackText"));
	Column->AddChildToVerticalBox(FeedbackText)->SetPadding(FMargin(0, 8, 0, 0));
}

void UCheatMenuWidget::PopulateResources()
{
	ResourceComboBox->ClearOptions();
	ResourceTypes.Reset();
	const UEnum* Enum = StaticEnum<EResourceType>();
	for (int32 Index = 0; Enum && Index < Enum->NumEnums() - 1; ++Index)
	{
		ResourceTypes.Add(static_cast<EResourceType>(Enum->GetValueByIndex(Index)));
		ResourceComboBox->AddOption(Enum->GetDisplayNameTextByIndex(Index).ToString());
	}
	if (ResourceComboBox->GetOptionCount() > 0) ResourceComboBox->SetSelectedIndex(0);
}

void UCheatMenuWidget::GiveSelectedResource()
{
#if !UE_BUILD_SHIPPING
	const int32 Index = ResourceComboBox ? ResourceComboBox->GetSelectedIndex() : INDEX_NONE;
	const int32 Amount = AmountSpinBox ? FMath::RoundToInt(AmountSpinBox->GetValue()) : 0;
	APlayerController* PC = GetOwningPlayer();
	USTPCheatManager* Cheats = PC ? Cast<USTPCheatManager>(PC->CheatManager) : nullptr;
	const bool bSuccess = ResourceTypes.IsValidIndex(Index) && Cheats && Cheats->GiveResource(ResourceTypes[Index], Amount);
	if (FeedbackText)
	{
		FeedbackText->SetText(bSuccess ? FText::Format(NSLOCTEXT("STPCheats", "Success", "Added {0}."), FText::AsNumber(Amount)) : NSLOCTEXT("STPCheats", "Failed", "Could not add resource."));
	}
#endif
}

void UCheatMenuWidget::CompleteUplinkObjective()
{
#if !UE_BUILD_SHIPPING
	APlayerController* PC = GetOwningPlayer();
	USTPCheatManager* Cheats = PC ? Cast<USTPCheatManager>(PC->CheatManager) : nullptr;
	const bool bSuccess = Cheats && Cheats->CompleteObjective(TEXT("mission_build_communication"));
	if (FeedbackText)
	{
		FeedbackText->SetText(bSuccess
			? NSLOCTEXT("STPCheats", "ObjectiveComplete", "ESTABLISH UPLINK completed; rewards granted.")
			: NSLOCTEXT("STPCheats", "ObjectiveCompleteFailed", "ESTABLISH UPLINK is not active."));
	}
#endif
}
