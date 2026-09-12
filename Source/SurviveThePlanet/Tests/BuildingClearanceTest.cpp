#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Base/BaseBuilding.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPBuildingClearanceTest,
	"SurviveThePlanet.Placement.BuildingClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPBuildingClearanceTest::RunTest(const FString& Parameters)
{
	const FName WorldName = MakeUniqueObjectName(
		nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	APlanetSurfaceManager* Surface = World->SpawnActor<APlanetSurfaceManager>();
	ABaseBuilding* First = World->SpawnActor<ABaseBuilding>();
	ABaseBuilding* Second = World->SpawnActor<ABaseBuilding>();
	if (!Surface || !First || !Second)
	{
		AddError(TEXT("Failed to create placement fixtures"));
		World->DestroyWorld(false);
		return false;
	}
	First->SetActorLocation(Surface->GetWorldLocationForCell(FSTPGridCell(90, 90)));
	Second->SetPlacementPreview(true);
	TestEqual(TEXT("Default two metre clearance"), Surface->GetBuildingClearanceCells(), 2);
	const FIntPoint BuildingFootprint = First->GetGridFootprint();
	TestTrue(TEXT("Reserve first building"), Surface->ReserveCells(First, FSTPGridCell(90, 90), BuildingFootprint));
	Surface->ReleaseCells(First);
	TestFalse(TEXT("Map-authored building blocks clearance without a reservation"),
		Surface->HasBuildingClearance(FSTPGridCell(92, 90), BuildingFootprint));
	TestTrue(TEXT("Placement preview does not block clearance"),
		Surface->HasBuildingClearance(FSTPGridCell(93, 90), BuildingFootprint));
	TestTrue(TEXT("Restore first building reservation"), Surface->ReserveCells(First, FSTPGridCell(90, 90), BuildingFootprint));
	TestFalse(TEXT("Touching edge rejected"), Surface->HasBuildingClearance(FSTPGridCell(91, 90), BuildingFootprint));
	TestFalse(TEXT("One-cell gap rejected"), Surface->HasBuildingClearance(FSTPGridCell(92, 90), BuildingFootprint));
	TestFalse(TEXT("Diagonal one-cell gap rejected"), Surface->HasBuildingClearance(FSTPGridCell(92, 92), BuildingFootprint));
	TestTrue(TEXT("Exact two-cell gap accepted"), Surface->HasBuildingClearance(FSTPGridCell(93, 90), BuildingFootprint));
	TestFalse(TEXT("Reservation enforces clearance"), Surface->ReserveCells(Second, FSTPGridCell(92, 90), BuildingFootprint));
	TestTrue(TEXT("Reserve second building at legal gap"), Surface->ReserveCells(Second, FSTPGridCell(93, 90), BuildingFootprint));
	TestTrue(TEXT("Two-cell drone corridor stays free"), Surface->CanOccupyCells(FSTPGridCell(91, 90), FIntPoint(2, 1)));
	TArray<FVector> Path;
	TestTrue(TEXT("Drone can route through the gap"), Surface->FindGridPath(
		Surface->GetWorldLocationForCell(FSTPGridCell(92, 87)), FSTPGridCell(92, 95), FIntPoint(1, 1), Path));
	Surface->ReleaseCells(First);
	Surface->ReleaseCells(Second);
	First->Destroy();
	Second->Destroy();
	TestTrue(TEXT("Destroyed buildings remove clearance restriction"),
		Surface->HasBuildingClearance(FSTPGridCell(91, 90), BuildingFootprint));
	World->DestroyWorld(false);
	return true;
}
#endif
