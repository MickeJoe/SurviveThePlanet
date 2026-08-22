#include "Gameplay/Cheats/CheatMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gameplay/Cheats/STPCheatManager.h"
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
#endif
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
