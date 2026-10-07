#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/Buildings/RemoteBase.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemoteBaseBuildTest,
	"SurviveThePlanet.Buildings.RemoteBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRemoteBaseBuildTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	AHexSectorGrid* Grid = World->SpawnActor<AHexSectorGrid>();
	Grid->GridRadius = 1;
	Grid->RebuildGrid();
	const FHexSector Sector = Grid->Sectors.Last();
	const FVector Location = Sector.WorldCenter;
	const FVector HalfX(250, 0, 0), HalfY(0, 250, 0);
	ARemoteBase* Preview = World->SpawnActor<ARemoteBase>(Location, FRotator::ZeroRotator);
	Preview->SetPlacementPreview(true);
	AEnergyModule* Factory = World->SpawnActor<AEnergyModule>(Location, FRotator::ZeroRotator);
	TestFalse(TEXT("Undiscovered sector rejects remote base"), Grid->CanPlaceBuilding(Preview, Location, HalfX, HalfY));
	Grid->SetSectorState(Sector.Id, ESectorState::Discovered);
	TestTrue(TEXT("Discovered empty sector accepts remote base"), Grid->CanPlaceBuilding(Preview, Location, HalfX, HalfY));
	TestFalse(TEXT("Ghost does not reserve a base slot"), Grid->HasSectorBase(Sector.Id));
	TestFalse(TEXT("Ordinary buildings need an established sector"), Grid->CanPlaceBuilding(Factory, Location, HalfX, HalfY));
	TestFalse(TEXT("Base footprint cannot span several sectors"), Grid->CanPlaceBuilding(Preview, Location, FVector(10000,0,0), HalfY));
	ARemoteBase* Base = World->SpawnActor<ARemoteBase>(Location, FRotator::ZeroRotator);
	Base->SetConstructionProgress(0.5f);
	TestTrue(TEXT("Unfinished base reserves its sector"), Grid->HasSectorBase(Sector.Id));
	TestFalse(TEXT("Second remote base rejected during construction"), Grid->CanPlaceBuilding(Preview, Location, HalfX, HalfY));
	FHexSector Current;
	Grid->GetSectorById(Sector.Id, Current);
	TestEqual(TEXT("Partial construction does not open sector"), Current.State, ESectorState::Discovered);
	Base->SetConstructionProgress(1.0f);
	Grid->GetSectorById(Sector.Id, Current);
	TestEqual(TEXT("Completed remote base establishes sector"), Current.State, ESectorState::Established);
	TestTrue(TEXT("Completed base permits ordinary construction"), Grid->CanPlaceBuilding(Factory, Location, HalfX, HalfY));
	Base->Destroy();
	Grid->GetSectorById(Sector.Id, Current);
	TestEqual(TEXT("Removed remote base closes construction access"), Current.State, ESectorState::Discovered);
	ABaseBuilding* Camp = World->SpawnActor<ABaseBuilding>(Location, FRotator::ZeroRotator);
	TestFalse(TEXT("Main base camp blocks a remote base"), Grid->CanPlaceBuilding(Preview, Location, HalfX, HalfY));
	Camp->Destroy();
	TestTrue(TEXT("Removing camp frees the only base slot"), Grid->CanPlaceBuilding(Preview, Location, HalfX, HalfY));
	UBuildingManagerSubsystem* Manager = World->GetSubsystem<UBuildingManagerSubsystem>();
	UBuildingDataAsset* Definition = Manager->GetDefinition(ESTPBuildTool::RemoteBase);
	if (TestNotNull(TEXT("Remote base is registered in catalog"), Definition))
	{
		TestEqual(TEXT("Infrastructure toolbar category"), Definition->BuildCategory, ESTPBuildCategory::Infrastructure);
		TestTrue(TEXT("Build button is visible and initially owned"), Definition->bShowInBuildToolbar && Definition->bBlueprintInitiallyOwned);
		TestNotNull(TEXT("Build button icon"), Definition->ToolbarIcon.Get());
		TestTrue(TEXT("Catalog class derives from remote base"), Manager->GetBuildingClass(ESTPBuildTool::RemoteBase)->IsChildOf(ARemoteBase::StaticClass()));
		TestTrue(TEXT("Connector rate is positive"), Definition->ConnectorsPerMeter > 0);
	}
	TestEqual(TEXT("Remote base fits a five metre footprint"), Preview->GetGridFootprint(), FIntPoint(5,5));
	TestNotNull(TEXT("Shared cable connection component"), Preview->PowerConnection.Get());
	TestEqual(TEXT("Select nearest source without hysteresis"), Preview->PowerConnection->SourceSwitchMargin, 0.0f);
	World->DestroyWorld(false);
	return true;
}
#endif
