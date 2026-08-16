#include "Gameplay/Buildings/CommunicationModule.h"

#include "UObject/ConstructorHelpers.h"

ACommunicationModule::ACommunicationModule()
{
	BuildingTag = TEXT("CommunicationModule");
	BuildingType = ESTPBuildingType::CommunicationModule;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultCommunicationModuleName", "Communication Module");
	BuildingDescription = NSLOCTEXT("SurviveThePlanet", "DefaultCommunicationModuleDescription", "Provides long-range communications for the colony.");
	EnergyProductionPerMinute = 0.0f;
	EnergyConsumptionPerMinute = 5.0f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(
		TEXT("/Game/Models/Buildings/CommunicationModule/CommunicationModuleMesh.CommunicationModuleMesh"));
	if (MeshFinder.Succeeded())
	{
		ModuleMesh = MeshFinder.Object;
	}
}
