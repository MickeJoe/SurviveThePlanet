#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/World/PlanetResourcePlacement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlanetResourcePlacementTest, "SurviveThePlanet.Resources.DistributionPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlanetResourcePlacementTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique));
	if (!TestNotNull(TEXT("World"), World)) return false;
	AHexSectorGrid* Grid = World->SpawnActor<AHexSectorGrid>();
	Grid->GridRadius = 1;
	Grid->RebuildGrid();
	for (const TCHAR* AssetPath : {TEXT("/Game/World/ResourceDistributions/DA_Rocky_Sparse.DA_Rocky_Sparse"),
		TEXT("/Game/World/ResourceDistributions/DA_Rocky_Rich.DA_Rocky_Rich")})
	{
		const auto* Asset = LoadObject<UPlanetResourceDistribution>(nullptr, AssetPath);
		if (TestNotNull(AssetPath, Asset))
			TestTrue(TEXT("Authored distribution fits the same layout"),
				UPlanetResourcePlacementComponent::PlaceResources(Grid, Asset, 17).bSuccess);
	}
	UPlanetResourceDistribution* Input = NewObject<UPlanetResourceDistribution>();
	FPlanetResourceRule Rule;
	Rule.Id = TEXT("Iron"); Rule.MinCount = Rule.MaxCount = 3;
	Input->Rules.Add(Rule);
	auto Generate = [&]() { return UPlanetResourcePlacementComponent::PlaceResources(Grid, Input, 17); };
	const auto A = Generate(); const auto B = Generate();
	TestTrue(TEXT("Authored default slots allow required deposits"), A.bSuccess);
	TestEqual(TEXT("Required count"), A.Deposits.Num(), 3);
	TestEqual(TEXT("Deterministic count"), A.Deposits.Num(), B.Deposits.Num());
	for (int32 I = 0; I < A.Deposits.Num() && I < B.Deposits.Num(); ++I)
	{
		TestEqual(TEXT("Deterministic location"), A.Deposits[I].Location, B.Deposits[I].Location);
		TestEqual(TEXT("Deterministic quantity"), A.Deposits[I].Quantity, B.Deposits[I].Quantity);
	}
	Input->Rules[0].MinCount = Input->Rules[0].MaxCount = 100;
	TestFalse(TEXT("Insufficient slots fail required placement"), Generate().bSuccess);
	Input->Rules[0].MinCount = Input->Rules[0].MaxCount = 2;
	Input->Rules[0].MinimumSpacing = 1000000;
	TestFalse(TEXT("Conflicting spacing fails"), Generate().bSuccess);
	Input->Rules[0].MinimumSpacing = 0;
	Input->Rules[0].Radius = 1000;
	TestFalse(TEXT("Oversized deposits fail"), Generate().bSuccess);
	Input->Rules[0].Radius = 100;
	Input->Rules[0].RequiredTemplateTags.Add(TEXT("MissingTag"));
	TestFalse(TEXT("Missing template tag fails"), Generate().bSuccess);
	Input->Rules[0].RequiredTemplateTags.Reset();
	Input->Rules[0].MaxDistanceFromHQ = 1;
	TestFalse(TEXT("HQ distance constraint fails"), Generate().bSuccess);
	Input->Rules[0].MaxDistanceFromHQ = 1000000;
	for (auto& Template : Grid->SectorTemplates)
		for (auto& Slot : Template.ResourceSlots) Slot.Location = FVector2D::ZeroVector;
	TestFalse(TEXT("Protected pockets/corridors reject slots"), Generate().bSuccess);
	Input->Rules[0].bGuaranteed = false;
	const auto Optional = Generate();
	TestTrue(TEXT("Optional shortage is allowed"), Optional.bSuccess);
	TestTrue(TEXT("Optional shortage is reported"), !Optional.Warnings.IsEmpty());
	Input->Rules[0].MinCount = -1;
	TestFalse(TEXT("Invalid input rejected"), Generate().bSuccess);
	World->DestroyWorld(false);
	return true;
}
#endif
