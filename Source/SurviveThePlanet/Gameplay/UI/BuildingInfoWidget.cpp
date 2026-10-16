#include "Gameplay/UI/BuildingInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Buildings/CrudeWorkshop.h"
#include "Gameplay/Resources/ResourceCatalog.h"
#include "SurviveThePlanetPlayerController.h"

void UDroneTypeSelectionButton::Configure(UBuildingInfoWidget* InOwnerWidget, TSubclassOf<ABaseDrone> InDroneClass, int32 InSlotIndex)
{
	OwnerWidget = InOwnerWidget;
	DroneClass = InDroneClass;
	SlotIndex = InSlotIndex;
	OnClicked.AddUniqueDynamic(this, &UDroneTypeSelectionButton::HandleClicked);
}

void UDroneTypeSelectionButton::HandleClicked()
{
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->AssignDroneOfClass(DroneClass, SlotIndex);
	}
}

void UDroneSlotButton::Configure(UBuildingInfoWidget* InOwnerWidget, int32 InSlotIndex)
{
	OwnerWidget = InOwnerWidget;
	SlotIndex = InSlotIndex;
	// Slot widgets are rebuilt whenever the selected building refreshes. OnPressed
	// fires immediately and remains reliable even if that refresh happens before release.
	OnPressed.AddUniqueDynamic(this, &UDroneSlotButton::HandleClicked);
	UE_LOG(LogTemp, Log, TEXT("Drone slot button created for slot %d."), SlotIndex);
}

void UDroneSlotButton::HandleClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Drone slot button pressed for slot %d. Owner valid: %s."),
		SlotIndex, OwnerWidget.IsValid() ? TEXT("true") : TEXT("false"));
	if (OwnerWidget.IsValid())
	{
		OwnerWidget->HandleDroneSlotClicked(SlotIndex);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Drone slot %d clicked without a valid BuildingInfoWidget owner."), SlotIndex);
	}
}

void UBuildingInfoWidget::SetBuilding(ABaseBuilding* NewBuilding)
{
	CloseDroneSelection();
	UnbindBuilding();
	Building = NewBuilding;
	SetVisibility(Building ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (!Building)
	{
		RefreshDroneSlots();
		return;
	}

	Building->OnDroneSlotsChanged.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleDroneSlotsChanged);
	Building->OnDroneAssignmentsChanged.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleDroneAssignmentsChanged);
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			CableNetworkManager = *It;
			CableNetworkManager->OnCableNetworkChanged.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleCableNetworkChanged);
			break;
		}
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			ResourceManager = *It;
			ResourceManager->OnResourceAmountChanged.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleResourceAmountChanged);
			break;
		}
	}

	BuildingNameText->SetText(Building->GetBuildingDisplayName());
	BuildingDescriptionText->SetVisibility(ESlateVisibility::Collapsed);

	if (UTexture2D* Thumbnail = Building->GetBuildingThumbnail())
	{
		BuildingImage->SetBrushFromTexture(Thumbnail, true);
		BuildingImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		BuildingImage->SetColorAndOpacity(FLinearColor::Transparent);
	}

	RefreshDroneSlots();
	BuildRecipeSelection();
	BuildProductionFlow();
	RefreshPowerStatus();
}

void UBuildingInfoWidget::RefreshPowerStatus()
{
	if (!PowerStatusText)
	{
		return;
	}
	const bool bConnected = IsValid(Building) && Building->IsConnectedToPowerGrid();
	const bool bOperational = bConnected && Building->IsOperational();
	PowerStatusText->SetVisibility(ESlateVisibility::Collapsed);
	if (PowerStatusImage)
	{
		const TCHAR* Path = !bConnected ? TEXT("/Game/UI/BuildingFlow/T_PowerDisconnected")
			: bOperational ? TEXT("/Game/UI/BuildingFlow/T_PowerConnected") : TEXT("/Game/UI/BuildingFlow/T_PowerWaiting");
		PowerStatusImage->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr, Path));
		PowerStatusImage->SetToolTipText(FText::FromString(!bConnected ? TEXT("Power grid disconnected")
			: bOperational ? TEXT("Connected and powered") : TEXT("Connected; waiting for power")));
	}
	RefreshProductionFlow();
}

void UBuildingInfoWidget::NativeDestruct()
{
	CloseDroneSelection();
	UnbindBuilding();
	Super::NativeDestruct();
}

void UBuildingInfoWidget::RefreshDroneSlots()
{
	if (!DroneAssignmentSection || !DroneSlotsBox)
	{
		return;
	}

	DroneSlotsBox->ClearChildren();
	const int32 MaxSlots = Building ? Building->GetMaxDroneSlots() : 0;
	DroneAssignmentSection->SetVisibility(MaxSlots > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (MaxSlots <= 0 || !WidgetTree)
	{
		return;
	}

	if (DroneCountText) DroneCountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d ASSIGNED"), Building->GetAssignedDroneCount(), MaxSlots)));
	const int32 UnlockedSlots = Building->GetUnlockedDroneSlots();
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		const bool bUnlocked = SlotIndex < UnlockedSlots;
		ABaseDrone* AssignedDrone = Building->GetAssignedDroneAtSlot(SlotIndex);

		UClass* CardClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/BuildingFlow/WBP_ProductionDroneSlot.WBP_ProductionDroneSlot_C"));
		UUserWidget* SlotSize = CardClass ? CreateWidget<UUserWidget>(GetOwningPlayer(), CardClass) : nullptr;
		if (!SlotSize) continue;
		SlotSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UImage* Icon = Cast<UImage>(SlotSize->GetWidgetFromName(TEXT("DroneIcon"))))
		{
			Icon->SetBrushFromTexture(AssignedDrone ? AssignedDrone->GetDroneThumbnail() : LockedDroneSlotIcon.Get());
			Icon->SetVisibility(AssignedDrone || !bUnlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UTextBlock* Label = Cast<UTextBlock>(SlotSize->GetWidgetFromName(TEXT("SlotLabel"))))
		{
			FSlateFontInfo Font = Label->GetFont();
			Font.Size = AssignedDrone ? 18 : bUnlocked ? 30 : 12;
			Label->SetFont(Font);
			if (auto* Position = Cast<UCanvasPanelSlot>(Label->Slot))
			{
				Position->SetPosition(FVector2D(0, AssignedDrone ? 59 : 20));
				Position->SetSize(FVector2D(126, AssignedDrone ? 24 : 50));
			}
			Label->SetText(FText::FromString(AssignedDrone ? FString::Printf(TEXT("%.0f%%"), AssignedDrone->GetWorkingRate(Building->GetDroneWorkType()) * 100.0f) : bUnlocked ? TEXT("+") : TEXT("LOCKED")));
		}

		UWidget* SlotWidget = SlotSize;
		if (bUnlocked)
		{
			UDroneSlotButton* SlotButton = WidgetTree->ConstructWidget<UDroneSlotButton>();
			SlotButton->Configure(this, SlotIndex);
			SlotButton->SetIsEnabled(true);
			SlotButton->SetVisibility(ESlateVisibility::Visible);
			SlotButton->SetBackgroundColor(FLinearColor::Transparent);
			SlotButton->SetContent(SlotSize);
			SlotWidget = SlotButton;
		}

		if (UHorizontalBoxSlot* HorizontalSlot = DroneSlotsBox->AddChildToHorizontalBox(SlotWidget))
		{
			HorizontalSlot->SetPadding(FMargin(0.0f, 0.0f, SlotIndex + 1 < MaxSlots ? 10.0f : 0.0f, 0.0f));
		}
	}
}

void UBuildingInfoWidget::HandleDroneSlotClicked(int32 SlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("Building info received drone slot %d click."), SlotIndex);
	if (!IsValid(Building) || SlotIndex < 0 || SlotIndex >= Building->GetUnlockedDroneSlots())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ignored drone slot click. Building valid: %s, slot: %d, unlocked slots: %d."),
			IsValid(Building) ? TEXT("true") : TEXT("false"), SlotIndex,
			IsValid(Building) ? Building->GetUnlockedDroneSlots() : 0);
		return;
	}

	if (Building->GetAssignedDroneAtSlot(SlotIndex))
	{
		CloseDroneSelection();
		Building->UnassignDroneAtSlot(SlotIndex);
		return;
	}

	OpenDroneSelection(SlotIndex);
}

void UBuildingInfoWidget::OpenDroneSelection(int32 SlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("Opening drone selection for slot %d."), SlotIndex);
	CloseDroneSelection();
	if (!IsValid(Building) || !GetOwningPlayer())
	{
		return;
	}

	if (!DroneSelectionWidgetClass || !DroneSelectionCardClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open drone selection: WBP_DroneSelectionPopup or WBP_DroneSelectionCard failed to load."));
		return;
	}

	DroneSelectionWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), DroneSelectionWidgetClass);
	if (!DroneSelectionWidget)
	{
		return;
	}

	UUniformGridPanel* OptionsGrid = Cast<UUniformGridPanel>(DroneSelectionWidget->GetWidgetFromName(TEXT("DroneOptionsGrid")));
	UButton* CloseButton = Cast<UButton>(DroneSelectionWidget->GetWidgetFromName(TEXT("CloseButton")));
	if (!OptionsGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not open drone selection: WBP_DroneSelectionPopup is missing DroneOptionsGrid."));
		DroneSelectionWidget->RemoveFromParent();
		DroneSelectionWidget = nullptr;
		return;
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UBuildingInfoWidget::CloseDroneSelection);
	}

	OptionsGrid->ClearChildren();
	TMap<UClass*, ABaseDrone*> RepresentativeByClass;
	TMap<UClass*, int32> AvailableCountByClass;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABaseDrone> It(World); It; ++It)
		{
			ABaseDrone* Drone = *It;
			if (IsValid(Drone))
			{
				RepresentativeByClass.FindOrAdd(Drone->GetClass()) = Drone;
				AvailableCountByClass.FindOrAdd(Drone->GetClass()) += Drone->IsAvailableForAssignment() ? 1 : 0;
			}
		}
	}

	TArray<UClass*> DroneClasses;
	RepresentativeByClass.GetKeys(DroneClasses);
	const ESTPDroneWorkType WorkType = Building->GetDroneWorkType();
	DroneClasses.Sort([WorkType](const UClass& A, const UClass& B)
	{
		const ABaseDrone* DroneA = Cast<ABaseDrone>(A.GetDefaultObject());
		const ABaseDrone* DroneB = Cast<ABaseDrone>(B.GetDefaultObject());
		return (DroneA ? DroneA->GetWorkingRate(WorkType) : 0.0f) > (DroneB ? DroneB->GetWorkingRate(WorkType) : 0.0f);
	});

	float BestRate = 0.0f;
	for (UClass* DroneClass : DroneClasses)
	{
		if (const ABaseDrone* Drone = Cast<ABaseDrone>(DroneClass->GetDefaultObject()))
		{
			BestRate = FMath::Max(BestRate, Drone->GetWorkingRate(WorkType));
		}
	}

	for (int32 Index = 0; Index < DroneClasses.Num(); ++Index)
	{
		UClass* DroneClass = DroneClasses[Index];
		ABaseDrone* Representative = RepresentativeByClass.FindRef(DroneClass);
		if (!Representative)
		{
			continue;
		}

		UUserWidget* Card = CreateWidget<UUserWidget>(GetOwningPlayer(), DroneSelectionCardClass);
		if (!Card)
		{
			continue;
		}

		const int32 AvailableCount = AvailableCountByClass.FindRef(DroneClass);
		const float Rate = Representative->GetWorkingRate(WorkType);
		if (UImage* Thumbnail = Cast<UImage>(Card->GetWidgetFromName(TEXT("DroneThumbnail"))))
		{
			Thumbnail->SetBrushFromTexture(Representative->GetDroneThumbnail(), true);
		}
		if (UTextBlock* NameText = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("DroneNameText"))))
		{
			NameText->SetText(Representative->GetDroneDisplayName());
		}
		if (UTextBlock* CountText = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("DroneCountText"))))
		{
			CountText->SetText(FText::Format(NSLOCTEXT("SurviveThePlanet", "DroneAvailableCount", "x{0}"), FText::AsNumber(AvailableCount)));
		}
		if (UTextBlock* EfficiencyText = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("EfficiencyText"))))
		{
			EfficiencyText->SetText(FText::Format(NSLOCTEXT("SurviveThePlanet", "DroneEfficiency", "{0}%"), FText::AsNumber(FMath::RoundToInt(Rate * 100.0f))));
			EfficiencyText->SetColorAndOpacity(FSlateColor(Rate <= 0.0f
				? FLinearColor(0.48f, 0.30f, 0.24f, 1.0f)
				: (Rate >= 1.0f ? FLinearColor(0.42f, 0.82f, 0.22f, 1.0f) : FLinearColor(1.0f, 0.58f, 0.08f, 1.0f))));
		}
		if (UTextBlock* BestText = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("BestText"))))
		{
			BestText->SetVisibility(BestRate > 0.0f && FMath::IsNearlyEqual(Rate, BestRate)
				? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (UDroneTypeSelectionButton* SelectButton = Cast<UDroneTypeSelectionButton>(Card->GetWidgetFromName(TEXT("SelectButton"))))
		{
			SelectButton->Configure(this, DroneClass, SlotIndex);
			SelectButton->SetIsEnabled(AvailableCount > 0 && Rate > 0.0f);
		}

		if (UUniformGridSlot* GridSlot = OptionsGrid->AddChildToUniformGrid(Card, Index / 2, Index % 2))
		{
			const bool bLastCardInOddRow = (DroneClasses.Num() % 2) != 0
				&& Index == DroneClasses.Num() - 1;
			GridSlot->SetHorizontalAlignment(bLastCardInOddRow ? HAlign_Left : HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	DroneSelectionWidget->AddToViewport(50);
	UE_LOG(LogTemp, Log, TEXT("Drone selection opened with %d drone type card(s)."), OptionsGrid->GetChildrenCount());
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	// WBP_BuildingPopup's root canvas fills the viewport, so using this widget's
	// geometry always returns x=0 and places a right-aligned selector off-screen.
	// Position from the visible panel itself instead.
	UWidget* BuildingPopupPanel = GetWidgetFromName(TEXT("PopupPanel"));
	if (!BuildingPopupPanel) BuildingPopupPanel = GetWidgetFromName(TEXT("PopupWidth"));
	const FGeometry& AnchorGeometry = BuildingPopupPanel
		? BuildingPopupPanel->GetCachedGeometry()
		: GetCachedGeometry();
	USlateBlueprintLibrary::LocalToViewport(this, AnchorGeometry, FVector2D::ZeroVector, PixelPosition, ViewportPosition);
	DroneSelectionWidget->SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
	DroneSelectionWidget->SetPositionInViewport(FVector2D(FMath::Max(0.0f, ViewportPosition.X - 10.0f), ViewportPosition.Y), false);
	UE_LOG(LogTemp, Log, TEXT("Drone selection positioned at viewport=(%.1f, %.1f), anchor=%s."),
		ViewportPosition.X, ViewportPosition.Y, *GetNameSafe(BuildingPopupPanel));
}

bool UBuildingInfoWidget::CloseDroneSelectionIfOpen()
{
	if (!DroneSelectionWidget || !DroneSelectionWidget->IsInViewport()) return false;
	CloseDroneSelection();
	return true;
}

void UBuildingInfoWidget::CloseDroneSelection()
{
	if (DroneSelectionWidget)
	{
		DroneSelectionWidget->RemoveFromParent();
		DroneSelectionWidget = nullptr;
	}
}

void UBuildingInfoWidget::AssignDroneOfClass(TSubclassOf<ABaseDrone> DroneClass, int32 SlotIndex)
{
	if (!IsValid(Building) || !DroneClass || SlotIndex < 0 || SlotIndex >= Building->GetUnlockedDroneSlots()
		|| Building->GetAssignedDroneAtSlot(SlotIndex))
	{
		CloseDroneSelection();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABaseDrone> It(World); It; ++It)
		{
			ABaseDrone* Drone = *It;
			if (IsValid(Drone) && Drone->IsA(DroneClass) && Drone->IsAvailableForAssignment()
				&& Building->TryAssignDrone(Drone, SlotIndex))
			{
				CloseDroneSelection();
				return;
			}
		}
	}
}

void UBuildingInfoWidget::UnbindBuilding()
{
	if (CableNetworkManager)
	{
		CableNetworkManager->OnCableNetworkChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleCableNetworkChanged);
		CableNetworkManager = nullptr;
	}
	if (ResourceManager)
	{
		ResourceManager->OnResourceAmountChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleResourceAmountChanged);
		ResourceManager = nullptr;
	}
	if (Building)
	{
		Building->OnDroneSlotsChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleDroneSlotsChanged);
		Building->OnDroneAssignmentsChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleDroneAssignmentsChanged);
	}
}

void UBuildingInfoWidget::HandleCableNetworkChanged()
{
	RefreshPowerStatus();
}

void UBuildingInfoWidget::HandleResourceAmountChanged(EResourceType ResourceType, int32 NewAmount)
{
	if (ResourceType == EResourceType::Energy)
	{
		RefreshPowerStatus();
	}
}

void UBuildingInfoWidget::HandleDroneSlotsChanged(int32 UnlockedSlots)
{
	RefreshDroneSlots();
}

void UBuildingInfoWidget::HandleDroneAssignmentsChanged()
{
	RefreshDroneSlots();
}

void UWorkshopRecipeButton::Configure(UBuildingInfoWidget* InOwner, int32 InRecipeIndex)
{
	OwnerWidget = InOwner;
	RecipeIndex = InRecipeIndex;
	OnClicked.AddUniqueDynamic(this, &UWorkshopRecipeButton::HandleClicked);
}

void UWorkshopRecipeButton::HandleClicked()
{
	if (OwnerWidget.IsValid()) OwnerWidget->SelectWorkshopRecipe(RecipeIndex);
}

void UBuildingInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (StartProductionButton) StartProductionButton->OnClicked.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleStartProduction);
	if (PauseProductionButton) PauseProductionButton->OnClicked.AddUniqueDynamic(this, &UBuildingInfoWidget::HandlePauseProduction);
	if (ClosePopupButton) ClosePopupButton->OnClicked.AddUniqueDynamic(this, &UBuildingInfoWidget::HandleClosePopup);
}

void UBuildingInfoWidget::HandleStartProduction()
{
	if (IsValid(Building)) Building->SetProductionEnabled(true);
	RefreshProductionFlow();
}

void UBuildingInfoWidget::HandlePauseProduction()
{
	if (IsValid(Building)) Building->SetProductionEnabled(false);
	RefreshProductionFlow();
}

void UBuildingInfoWidget::HandleClosePopup()
{
	if (auto* Controller = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) Controller->SetSelectedActor(nullptr);
	SetBuilding(nullptr);
}

void UBuildingInfoWidget::SelectWorkshopRecipe(int32 RecipeIndex)
{
	ACrudeWorkshop* Workshop = Cast<ACrudeWorkshop>(Building);
	if (!Workshop || RecipeIndex < 0 || RecipeIndex > 2) return;
	Workshop->SetRecipe(static_cast<ECrudeWorkshopRecipe>(RecipeIndex));
	BuildProductionFlow();
	RefreshRecipeSelection();
}

void UBuildingInfoWidget::BuildRecipeSelection()
{
	RecipeCards.Reset();
	if (RecipeButtonsBox) RecipeButtonsBox->ClearChildren();
	ACrudeWorkshop* Workshop = Cast<ACrudeWorkshop>(Building);
	if (RecipeSelectionSection) RecipeSelectionSection->SetVisibility(Workshop ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!Workshop || !RecipeButtonsBox) return;
	UClass* CardClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/BuildingFlow/WBP_ProductionRecipeCard.WBP_ProductionRecipeCard_C"));
	for (int32 Index = 0; Index < 3 && CardClass; ++Index)
	{
		UUserWidget* Card = CreateWidget<UUserWidget>(GetOwningPlayer(), CardClass);
		if (!Card) continue;
		const auto Recipe = static_cast<ECrudeWorkshopRecipe>(Index);
		if (auto* Button = Cast<UWorkshopRecipeButton>(Card->GetWidgetFromName(TEXT("RecipeButton")))) Button->Configure(this, Index);
		if (auto* Icon = Cast<UImage>(Card->GetWidgetFromName(TEXT("RecipeIcon")))) Icon->SetBrushFromTexture(UResourceCatalog::GetResourceIcon(ACrudeWorkshop::GetOutputForRecipe(Recipe)));
		if (auto* Label = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("RecipeName")))) Label->SetText(FText::FromString(Index == 0 ? TEXT("CONCRETE") : Index == 1 ? TEXT("STEEL") : TEXT("COPPER")));
		const int32 Seconds = FMath::RoundToInt(Workshop->GetCycleSecondsForRecipe(Recipe));
		if (auto* Time = Cast<UTextBlock>(Card->GetWidgetFromName(TEXT("RecipeTime")))) Time->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60)));
		RecipeButtonsBox->AddChildToHorizontalBox(Card)->SetPadding(FMargin(0, 0, 8, 0));
		RecipeCards.Add(Card);
	}
	RefreshRecipeSelection();
}

void UBuildingInfoWidget::RefreshRecipeSelection()
{
	const ACrudeWorkshop* Workshop = Cast<ACrudeWorkshop>(Building);
	if (!Workshop) return;
	for (int32 Index = 0; Index < RecipeCards.Num(); ++Index)
		if (UBorder* Selection = Cast<UBorder>(RecipeCards[Index]->GetWidgetFromName(TEXT("SelectionBorder"))))
			Selection->SetBrushColor(Index == static_cast<int32>(Workshop->GetSelectedRecipe()) ? FLinearColor(0.02f, 0.8f, 1.0f) : FLinearColor(0.16f, 0.25f, 0.28f));
}
