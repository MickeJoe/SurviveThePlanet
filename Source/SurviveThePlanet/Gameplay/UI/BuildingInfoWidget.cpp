#include "Gameplay/UI/BuildingInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/BaseDrone.h"

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

	BuildingNameText->SetText(Building->GetBuildingDisplayName());
	BuildingDescriptionText->SetText(Building->GetBuildingDescription());

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

	const int32 UnlockedSlots = Building->GetUnlockedDroneSlots();
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		const bool bUnlocked = SlotIndex < UnlockedSlots;
		ABaseDrone* AssignedDrone = Building->GetAssignedDroneAtSlot(SlotIndex);

		USizeBox* SlotSize = WidgetTree->ConstructWidget<USizeBox>();
		SlotSize->SetWidthOverride(82.0f);
		SlotSize->SetHeightOverride(82.0f);
		// The wrapping UDroneSlotButton owns the full hit area. Its visual children
		// must never become the Slate hit target themselves.
		SlotSize->SetVisibility(ESlateVisibility::HitTestInvisible);

		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>();
		SlotBorder->SetPadding(FMargin(10.0f));
		SlotBorder->SetHorizontalAlignment(HAlign_Center);
		SlotBorder->SetVerticalAlignment(VAlign_Center);
		SlotBorder->SetBrushColor(bUnlocked
			? (AssignedDrone
				? FLinearColor(0.07f, 0.22f, 0.10f, 1.0f)
				: FLinearColor(0.055f, 0.16f, 0.075f, 1.0f))
			: FLinearColor(0.035f, 0.045f, 0.05f, 1.0f));
		SlotSize->SetContent(SlotBorder);

		if (bUnlocked && AssignedDrone && AssignedDrone->GetDroneThumbnail())
		{
			UImage* DroneImage = WidgetTree->ConstructWidget<UImage>();
			DroneImage->SetBrushFromTexture(AssignedDrone->GetDroneThumbnail(), true);
			DroneImage->SetColorAndOpacity(FLinearColor::White);
			SlotBorder->SetContent(DroneImage);
		}
		else if (!bUnlocked && LockedDroneSlotIcon)
		{
			USizeBox* LockIconSize = WidgetTree->ConstructWidget<USizeBox>();
			LockIconSize->SetWidthOverride(42.0f);
			LockIconSize->SetHeightOverride(48.0f);

			UImage* LockIcon = WidgetTree->ConstructWidget<UImage>();
			LockIcon->SetBrushFromTexture(LockedDroneSlotIcon, true);
			LockIcon->SetColorAndOpacity(FLinearColor(0.72f, 0.74f, 0.75f, 0.9f));
			LockIconSize->SetContent(LockIcon);
			SlotBorder->SetContent(LockIconSize);
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

	if (!DroneSelectionWidgetClass)
	{
		DroneSelectionWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_DroneSelectionPopup.WBP_DroneSelectionPopup_C"));
	}
	if (!DroneSelectionCardClass)
	{
		DroneSelectionCardClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_DroneSelectionCard.WBP_DroneSelectionCard_C"));
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
	const FGeometry& AnchorGeometry = BuildingPopupPanel
		? BuildingPopupPanel->GetCachedGeometry()
		: GetCachedGeometry();
	USlateBlueprintLibrary::LocalToViewport(this, AnchorGeometry, FVector2D::ZeroVector, PixelPosition, ViewportPosition);
	DroneSelectionWidget->SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
	DroneSelectionWidget->SetPositionInViewport(FVector2D(FMath::Max(0.0f, ViewportPosition.X - 10.0f), ViewportPosition.Y), false);
	UE_LOG(LogTemp, Log, TEXT("Drone selection positioned at viewport=(%.1f, %.1f), anchor=%s."),
		ViewportPosition.X, ViewportPosition.Y, *GetNameSafe(BuildingPopupPanel));
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
	if (Building)
	{
		Building->OnDroneSlotsChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleDroneSlotsChanged);
		Building->OnDroneAssignmentsChanged.RemoveDynamic(this, &UBuildingInfoWidget::HandleDroneAssignmentsChanged);
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
