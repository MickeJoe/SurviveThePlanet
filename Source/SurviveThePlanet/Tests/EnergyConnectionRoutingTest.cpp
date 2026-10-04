#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Gameplay/Energy/EnergyConnectionRouting.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyConnectionRoutingTest, "SurviveThePlanet.Energy.ConnectionRouting",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnergyConnectionRoutingTest::RunTest(const FString& Parameters)
{
 const FVector2D Start(0,0), End(3000,0);
 TArray<FVector2D> Path;
 TArray<FBox2D> Obstacles;
 TestTrue(TEXT("Empty corridor routes directly"), FEnergyConnectionRouting::FindPath(Start, End, Obstacles, Path));
 TestEqual(TEXT("No unnecessary bends"), Path.Num(), 2);
 Obstacles.Add(FBox2D(FVector2D(900,-300), FVector2D(1400,300)));
 Obstacles.Add(FBox2D(FVector2D(1300,-100), FVector2D(2000,600)));
 TestTrue(TEXT("Overlapping footprints have a detour"), FEnergyConnectionRouting::FindPath(Start, End, Obstacles, Path));
 TestTrue(TEXT("Detour includes support bends"), Path.Num() > 2);
 for (int32 Index=0; Index+1<Path.Num(); ++Index)
  TestTrue(TEXT("Every cable leg avoids building footprints"), FEnergyConnectionRouting::IsSegmentClear(Path[Index],Path[Index+1],Obstacles));
 Obstacles.Add(FBox2D(FVector2D(-100,-100), FVector2D(100,100)));
 TestFalse(TEXT("Blocked endpoint never falls back through a building"), FEnergyConnectionRouting::FindPath(Start, End, Obstacles, Path));
 return true;
}
#endif
