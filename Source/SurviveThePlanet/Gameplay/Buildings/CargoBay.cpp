#include "Gameplay/Buildings/CargoBay.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ACargoBay::ACargoBay()
{
	BuildingTag = TEXT("CargoBay");
	BuildingType = ESTPBuildingType::CargoBay;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultCargoBayName", "Cargo Bay");
	BuildingDescription = NSLOCTEXT("SurviveThePlanet", "DefaultCargoBayDescription", "Stores cargo and supports colony logistics.");
	EnergyConsumptionPerMinute = 3.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(
		TEXT("/Game/Models/Buildings/CargoBay/CargoBayMesh.CargoBayMesh"));
	if (MeshFinder.Succeeded())
	{
		ModuleMesh = MeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> ThumbnailFinder(
		TEXT("/Game/UI/Images/CargoBayBuildIcon.CargoBayBuildIcon"));
	if (ThumbnailFinder.Succeeded())
	{
		BuildingThumbnail = ThumbnailFinder.Object;
	}
}
