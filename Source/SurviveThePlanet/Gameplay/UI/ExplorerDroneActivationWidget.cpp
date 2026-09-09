#include "Gameplay/UI/ExplorerDroneActivationWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Gameplay/Drones/ExplorerDrone.h"
#include "SurviveThePlanetPlayerController.h"

void UExplorerDroneActivationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (LaunchButton) LaunchButton->OnClicked.AddUniqueDynamic(this, &UExplorerDroneActivationWidget::HandleLaunchClicked);
	RefreshFromSelection();
}

void UExplorerDroneActivationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshFromSelection();
}

void UExplorerDroneActivationWidget::RefreshFromSelection()
{
	ASurviveThePlanetPlayerController* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer());
	AExplorerDrone* NewDrone = PC ? Cast<AExplorerDrone>(PC->GetSelectedActor()) : nullptr;
	if (SelectedExplorerDrone != NewDrone)
	{
		SelectedExplorerDrone = NewDrone;
	}
	const bool bShow = IsValid(SelectedExplorerDrone) && !SelectedExplorerDrone->IsExploring();
	SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShow) return;
	if (DroneTitleText) DroneTitleText->SetText(SelectedExplorerDrone->GetDroneDisplayName().ToUpper());
	if (DroneThumbnailImage) DroneThumbnailImage->SetBrushFromTexture(SelectedExplorerDrone->GetDroneThumbnail(), false);
	const bool bCanLaunch = SelectedExplorerDrone->CanActivateExploration();
	if (LaunchButton) LaunchButton->SetIsEnabled(bCanLaunch);
	if (StatusText) StatusText->SetText(bCanLaunch ? FText::FromString(TEXT("REVEALS 3 NEAREST SECTORS")) : FText::FromString(TEXT("NO UNDISCOVERED SECTORS")));
}

void UExplorerDroneActivationWidget::HandleLaunchClicked()
{
	if (ASurviveThePlanetPlayerController* PC = Cast<ASurviveThePlanetPlayerController>(GetOwningPlayer())) PC->TryActivateSelectedExplorerDrone();
}
