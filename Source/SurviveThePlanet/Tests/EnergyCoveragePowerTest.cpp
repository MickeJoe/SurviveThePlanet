#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/WaterCollector.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyCoveragePowerTest, "SurviveThePlanet.Energy.CoveragePower",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnergyCoveragePowerTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 if (!TestNotNull(TEXT("World"),World)) return false;
 auto* CoverageSystem = World->GetSubsystem<UEnergyCoverageSubsystem>();
 auto* Buildings = World->GetSubsystem<UBuildingManagerSubsystem>();
 CoverageSystem->OnWorldBeginPlay(*World);
 ACableNetworkManager* Grid = nullptr;
 int32 ManagerCount = 0;
 for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
 {
  Grid = *It;
  ++ManagerCount;
 }
 TestEqual(TEXT("Game startup creates the energy manager without a map actor"),ManagerCount,1);
 if (!TestNotNull(TEXT("Startup energy manager"),Grid))
 {
  World->DestroyWorld(false);
  return false;
 }
 UWorld* ExistingWorld = UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 ExistingWorld->SpawnActor<ACableNetworkManager>();
 ExistingWorld->GetSubsystem<UEnergyCoverageSubsystem>()->OnWorldBeginPlay(*ExistingWorld);
 ManagerCount = 0;
 for (TActorIterator<ACableNetworkManager> It(ExistingWorld); It; ++It) ++ManagerCount;
 TestEqual(TEXT("Existing map energy manager is reused"),ManagerCount,1);
 ExistingWorld->DestroyWorld(false);
 auto* Resources = World->SpawnActor<AResourceManager>();
 Resources->SetResourceAmount(EResourceType::Energy,1000);
 auto* Weather = World->SpawnActor<APlanetWeatherManager>();
 FPlanetWeatherState Rain;
 Rain.PrecipitationPercent = 80;
 Weather->SetWeatherImmediately(Rain);
 auto* Camp = World->SpawnActor<ABaseBuilding>();
 Camp->SetConstructionProgress(1);
 auto* CampCoverage = NewObject<UEnergyCoverageComponent>(Camp);
 CampCoverage->SetupAttachment(Camp->GetRootComponent());
 CampCoverage->CoverageRadius = 1000;
 CampCoverage->RegisterComponent();
 CoverageSystem->RegisterSource(CampCoverage);
 auto MakeExtender = [&](const FVector& Location)
 {
  auto* Building = World->SpawnActor<ABaseBuilding>(Buildings->GetBuildingClass(ESTPBuildTool::EnergyExtender),Location,FRotator::ZeroRotator);
  Building->SetConstructionProgress(1);
  auto* Source = Building->FindComponentByClass<UEnergyCoverageComponent>();
  CoverageSystem->RegisterSource(Source);
  auto* Connection = NewObject<UEnergyConnectionComponent>(Building);
  Connection->SetupAttachment(Building->GetRootComponent());
  Connection->RegisterComponent();
  Connection->UpdateRoute(false,true);
  return Building;
 };
 auto* First = MakeExtender(FVector(4000,0,186.55));
 auto* Second = MakeExtender(FVector(8000,0,186.55));
 TestEqual(TEXT("Second extender connects through first"),Second->FindComponentByClass<UEnergyConnectionComponent>()->GetSourceActor(),static_cast<AActor*>(First));
 auto* Water = World->SpawnActor<AWaterCollector>(AWaterCollector::StaticClass(),FVector(500,0,10000),FRotator::ZeroRotator);
 Water->SetConstructionProgress(1);
 auto* Mine = World->SpawnActor<AMiningMachine>(AMiningMachine::StaticClass(),FVector(8500,0,186.55),FRotator::ZeroRotator);
 Mine->SetConstructionProgress(1);
 Grid->RefreshEnergyGrid();
 TestTrue(TEXT("Water collector connects by ground radius without manual cable cells"),Water->IsConnectedToPowerGrid());
 TestTrue(TEXT("Covered collector is powered by stored energy"),Water->IsOperational());
 TestTrue(TEXT("Mine inside chained extender coverage connects"),Mine->IsConnectedToPowerGrid());
 TestTrue(TEXT("Covered mine passes its operating power gate"),Mine->IsOperational());
 TestTrue(TEXT("Extender itself is connected outside original coverage"),Second->IsConnectedToPowerGrid());
 TestTrue(TEXT("Rainy collector has real energy demand"),Water->GetEnergyConsumptionPerMinute()>0);
 const int32 StoredBefore = Resources->GetResourceAmount(EResourceType::Energy);
 Grid->Tick(60.0f);
 TestEqual(TEXT("Powered consumers spend real stored energy"),Resources->GetResourceAmount(EResourceType::Energy),StoredBefore-FMath::TruncToInt(Grid->GetGridConsumptionPerMinute()));
 TestEqual(TEXT("Covered consumption enters the real energy budget"),Grid->GetGridConsumptionPerMinute(),Water->GetEnergyConsumptionPerMinute()+Mine->GetEnergyConsumptionPerMinute());
 TestTrue(TEXT("Radius includes its boundary"),CoverageSystem->IsLocationConnectedToPowerGrid(FVector(1000,0,0)));
 TestFalse(TEXT("Outside radius is disconnected"),CoverageSystem->IsLocationConnectedToPowerGrid(FVector(1001,0,0)));
 Water->SetActorLocation(FVector(12000,0,186.55));
 Grid->RefreshEnergyGrid();
 TestFalse(TEXT("Collector outside all areas disconnects"),Water->IsConnectedToPowerGrid());
 TestEqual(TEXT("Disconnected collector consumes no grid energy"),Grid->GetGridConsumptionPerMinute(),Mine->GetEnergyConsumptionPerMinute());
 Water->SetActorLocation(FVector(500,0,186.55));
 Water->SetPlacementPreview(true);
 Grid->RefreshEnergyGrid();
 TestFalse(TEXT("Placement ghost does not receive power"),Water->IsConnectedToPowerGrid());
 Water->SetPlacementPreview(false);
 Water->SetConstructionProgress(1);
 Resources->SetResourceAmount(EResourceType::Energy,0);
 Grid->RefreshEnergyGrid();
 TestTrue(TEXT("Energy shortage retains physical grid connection"),Water->IsConnectedToPowerGrid());
 TestFalse(FString::Printf(TEXT("Energy shortage stops collector operation [production=%.1f consumption=%.1f stock=%d waterDemand=%.1f]"),Grid->GetGridProductionPerMinute(),Grid->GetGridConsumptionPerMinute(),Resources->GetResourceAmount(EResourceType::Energy),Water->GetEnergyConsumptionPerMinute()),Water->IsOperational());
 Resources->SetResourceAmount(EResourceType::Energy,1000);
 Grid->RefreshEnergyGrid();
 TestTrue(TEXT("Stored energy restores collector operation"),Water->IsOperational());
 CoverageSystem->SetCoverageVisualizationVisible(true);
 First->SetConstructionProgress(0.5f);
 Grid->RefreshEnergyGrid();
 TestFalse(TEXT("Unfinished intermediate extender breaks downstream power"),Mine->IsConnectedToPowerGrid());
 TestFalse(TEXT("Disconnected descendant does not draw an active coverage circle"),Second->FindComponentByClass<UEnergyCoverageComponent>()->IsCoverageVisualizationVisible());
 First->SetConstructionProgress(1);
 Grid->RefreshEnergyGrid();
 TestTrue(TEXT("Completing intermediate extender restores downstream power"),Mine->IsConnectedToPowerGrid());
 CoverageSystem->UnregisterSource(CampCoverage);
 Grid->RefreshEnergyGrid();
 TestFalse(TEXT("Removing Base Camp disconnects its extender branch"),Mine->IsConnectedToPowerGrid());
 TestFalse(TEXT("Root loss disconnects ordinary buildings"),Water->IsConnectedToPowerGrid());
 TestFalse(TEXT("Root loss hides downstream powered coverage"),Second->FindComponentByClass<UEnergyCoverageComponent>()->IsCoverageVisualizationVisible());
 World->DestroyWorld(false);
 return true;
}
#endif
