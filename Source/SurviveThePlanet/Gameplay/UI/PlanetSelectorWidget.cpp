#include "PlanetSelectorWidget.h"
#include "Gameplay/Planet/PlanetDefinition.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
void UPlanetSelectorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	NexarisButton->OnClicked.AddUniqueDynamic(this, &UPlanetSelectorWidget::SelectNexaris);
	AridusButton->OnClicked.AddUniqueDynamic(this, &UPlanetSelectorWidget::SelectAridus);
	BorealisButton->OnClicked.AddUniqueDynamic(this, &UPlanetSelectorWidget::SelectBorealis);
	LandButton->OnClicked.AddUniqueDynamic(this, &UPlanetSelectorWidget::Land);
	SelectNexaris();
}
void UPlanetSelectorWidget::SelectNexaris() { SelectPlanet(TEXT("/Game/Data/Planet/DA_Nexaris.DA_Nexaris")); }
void UPlanetSelectorWidget::SelectAridus() { SelectPlanet(TEXT("/Game/Data/Planet/DA_Aridus.DA_Aridus")); }
void UPlanetSelectorWidget::SelectBorealis() { SelectPlanet(TEXT("/Game/Data/Planet/DA_Borealis.DA_Borealis")); }
void UPlanetSelectorWidget::SelectPlanet(const TCHAR* AssetPath)
{
	SelectedPlanet = LoadObject<UPlanetDefinition>(nullptr, AssetPath);
	LandButton->SetIsEnabled(SelectedPlanet && !SelectedPlanet->LandingMap.IsNull());
	if (!SelectedPlanet) return;
	PlanetName->SetText(SelectedPlanet->DisplayName);
	const auto& W = SelectedPlanet->Weather;
	PlanetDetails->SetText(FText::FromString(FString::Printf(
		TEXT("%s\n\nCLIMATE AT LANDING\n\nWind     %.1f - %.1f m/s\nRain     %.1f - %.1f mm/h\nSun      %.0f - %.0f %%\n\nWet weather probability   %.0f %%\nMean temperature   %.0f C\nRotation period   %.0f hours\nLanding latitude   %.0f degrees\n\nLIGHT / DARK AT LANDING\nDaylight   %.1f h\nDawn + dusk   %.1f h\nDark night   %.1f h\nClouds and rain reduce daylight."),
		*SelectedPlanet->ClimateDescription.ToString(), W.WindRange.Minimum, W.WindRange.Maximum,
		W.PrecipitationRange.Minimum, W.PrecipitationRange.Maximum, W.SunRange.Minimum, W.SunRange.Maximum,
		SelectedPlanet->RainProbability * 100, SelectedPlanet->MeanTemperatureCelsius,
		SelectedPlanet->GetDayLengthMinutes() / 60.0, SelectedPlanet->LandingLatitudeDegrees,
		SelectedPlanet->GetLightHours(),
		SelectedPlanet->GetLightHours(-6.0f) - SelectedPlanet->GetLightHours(),
		SelectedPlanet->GetDayLengthMinutes() / 60.0 - SelectedPlanet->GetLightHours(-6.0f))));
	NexarisButton->SetBackgroundColor(SelectedPlanet->DisplayName.ToString() == TEXT("NEXARIS") ? FLinearColor(0.1f,0.8f,1) : FLinearColor(0.04f,0.25f,0.32f));
	AridusButton->SetBackgroundColor(SelectedPlanet->DisplayName.ToString() == TEXT("ARIDUS") ? FLinearColor(1,0.6f,0.2f) : FLinearColor(0.3f,0.15f,0.05f));
	BorealisButton->SetBackgroundColor(SelectedPlanet->DisplayName.ToString() == TEXT("BOREALIS") ? FLinearColor(0.6f,0.8f,1) : FLinearColor(0.15f,0.22f,0.35f));
}
void UPlanetSelectorWidget::Land()
{
	if (!SelectedPlanet || SelectedPlanet->LandingMap.IsNull()) return;
	const FString Package = SelectedPlanet->LandingMap.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::DoesPackageExist(Package)) return;
	LandButton->SetIsEnabled(false);
	GetOwningPlayer()->SetInputMode(FInputModeGameAndUI());
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UGameplayStatics::OpenLevel(this, FName(*Package));
}
