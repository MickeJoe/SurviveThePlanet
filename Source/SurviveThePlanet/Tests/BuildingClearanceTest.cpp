#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Base/BaseBuilding.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPBuildingClearanceTest,
	"SurviveThePlanet.Placement.BuildingClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPBuildingClearanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, nullptr, true);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	World->InitializeNewWorld(UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false));
	APlanetSurfaceManager* Surface = World->SpawnActor<APlanetSurfaceManager>();
	ABaseBuilding* First = World->SpawnActor<ABaseBuilding>();
	ABaseBuilding* Second = World->SpawnActor<ABaseBuilding>();
	if (!Surface || !First || !Second)
	{
		AddError(TEXT("Failed to create placement fixtures"));
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Default two metre clearance"), Surface->GetBuildingClearanceCells(), 2);
	TestTrue(TEXT("Reserve first building"), Surface->ReserveCells(First, FSTPGridCell(90, 90), FIntPoint(2, 2)));
	TestFalse(TEXT("Touching edge rejected"), Surface->HasBuildingClearance(FSTPGridCell(92, 90), FIntPoint(2, 2)));
	TestFalse(TEXT("One-cell gap rejected"), Surface->HasBuildingClearance(FSTPGridCell(93, 90), FIntPoint(2, 2)));
	TestFalse(TEXT("Diagonal one-cell gap rejected"), Surface->HasBuildingClearance(FSTPGridCell(93, 93), FIntPoint(2, 2)));
	TestTrue(TEXT("Exact two-cell gap accepted"), Surface->HasBuildingClearance(FSTPGridCell(94, 90), FIntPoint(2, 2)));
	TestFalse(TEXT("Reservation enforces clearance"), Surface->ReserveCells(Second, FSTPGridCell(93, 90), FIntPoint(2, 2)));
	TestTrue(TEXT("Reserve second building at legal gap"), Surface->ReserveCells(Second, FSTPGridCell(94, 90), FIntPoint(2, 2)));
	TestTrue(TEXT("Two-cell drone corridor stays free"), Surface->CanOccupyCells(FSTPGridCell(92, 90), FIntPoint(2, 2)));
	TArray<FVector> Path;
	TestTrue(TEXT("Drone can route through the gap"), Surface->FindGridPath(
		Surface->GetWorldLocationForCell(FSTPGridCell(92, 87)), FSTPGridCell(92, 95), FIntPoint(1, 1), Path));
	Surface->ReleaseCells(First);
	Surface->ReleaseCells(Second);
	TestTrue(TEXT("Release removes clearance restriction"), Surface->HasBuildingClearance(FSTPGridCell(91, 90), FIntPoint(2, 2)));
	World->DestroyWorld(false);
	return true;
}
#endif