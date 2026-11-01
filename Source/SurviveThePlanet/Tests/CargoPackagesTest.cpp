#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Buildings/ThermalProcessingBuilding.h"
#include "Gameplay/Buildings/ResourceProductionComponent.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCargoPackageOrdersTest, "SurviveThePlanet.CargoPackages.Orders",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCargoPackageOrdersTest::RunTest(const FString& Parameters)
{
 TSet<FName> SeenPackages;
 const int32 Orders[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
 for (int32 Run = 0; Run < 24; ++Run)
 {
  auto* GameInstance = NewObject<UGameInstance>(GEngine);
  GameInstance->InitializeStandalone(MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
  UWorld* World = GameInstance->GetWorld();
  auto* Blueprints = GameInstance->GetSubsystem<UBuildingBlueprintSubsystem>();
  auto* Objectives = World->GetSubsystem<UObjectiveSubsystem>();
  auto* Resources = World->SpawnActor<AResourceManager>();
  Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
  Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
  const FName Package = Objectives->GetSelectedCargoPackage();
  SeenPackages.Add(Package);
  TestTrue(TEXT("Uplink unlocks reliable water supply"),Blueprints->OwnsBlueprintById(TEXT("GroundwaterPump")));
  TestTrue(TEXT("Uplink unlocks silica extraction"),Blueprints->OwnsBlueprintById(TEXT("SilicaExtractor")));
  TestTrue(TEXT("Package unlocks Polymer Plant"),Blueprints->OwnsBlueprintById(TEXT("PolymerPlant")));
  TestTrue(TEXT("Package unlocks Water Purifier"),Blueprints->OwnsBlueprintById(TEXT("WaterPurifier")));
  TestTrue(TEXT("Package unlocks its third blueprint"),Blueprints->OwnsBlueprintById(Package == TEXT("A") ? TEXT("Glassworks") : TEXT("FuelProcessor")));
  TestFalse(TEXT("Other package blueprint stays locked"),Blueprints->OwnsBlueprintById(Package == TEXT("A") ? TEXT("FuelProcessor") : TEXT("Glassworks")));
  TestEqual(TEXT("Finite startup coal reserve"),Resources->GetResourceAmount(EResourceType::Coal),40);
  TestEqual(TEXT("Cargo does not assign missions without a player choice"),Objectives->GetActiveObjectives().Num(),0);
  Objectives->UpdateMissionClock(2000);
  const auto Packets = Objectives->GetMissionChoiceGroups();
  for (int32 Index=0; Index<3; ++Index)
   TestTrue(TEXT("Player accepts independent packet"),Objectives->ConfirmMissionChoiceSlot(Packets[Index].Id,0,Packets[Index].Slots[0].ObjectiveIds[0]));
  const auto Active = Objectives->GetActiveObjectives();
  if (!TestEqual(TEXT("Exactly three initial requests"),Active.Num(),3)) { World->DestroyWorld(false); return false; }
  World->SpawnActor<ACargoBay>()->SetConstructionProgress(1);
  for (int32 Step = 0; Step < 3; ++Step)
  {
   const FName Id = Active[Orders[Run % 6][Step]].ObjectiveId;
   FSTPObjectiveDefinition Def;
   Objectives->GetObjectiveDefinition(Id,Def);
   const auto& Cost = Def.Conditions[0];
   Resources->SetResourceAmount(Cost.ResourceType,Cost.RequiredAmount-1);
   TestFalse(TEXT("Cannot ship missing inventory"),Objectives->DeliverObjective(Id));
   Resources->SetResourceAmount(Cost.ResourceType,Cost.RequiredAmount);
   TestTrue(TEXT("Selected order is accepted"),Objectives->DeliverObjective(Id));
   TestEqual(TEXT("Inventory paid exactly once"),Resources->GetResourceAmount(Cost.ResourceType),0);
   TestFalse(TEXT("No repeat rewards"),Objectives->DeliverObjective(Id));
   Objectives->Tick(10000);
   TestEqual(TEXT("Other requests retained"),Objectives->GetActiveObjectives().Num(),2-Step);
  }
  TestEqual(TEXT("Stage does not require extra hidden objectives"),World->GetSubsystem<UMissionConfidenceSubsystem>()->GetMissionProgress(),5);
  Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
  TestEqual(TEXT("Package never rerolls"),Objectives->GetSelectedCargoPackage(),Package);
  GameInstance->Shutdown();
  GEngine->DestroyWorldContext(World);
  World->DestroyWorld(false);
 }
 TestEqual(TEXT("Both packages exercised"),SeenPackages.Num(),2);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCargoProductionRecipesTest,"SurviveThePlanet.CargoPackages.Recipes",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCargoProductionRecipesTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 auto* Resources = World->SpawnActor<AResourceManager>();
 const TCHAR* Keys[] = {TEXT("Glassworks"),TEXT("FuelProcessor"),TEXT("WaterPurifier"),TEXT("GroundwaterPump")};
 const EResourceType Outputs[] = {EResourceType::Glass,EResourceType::SyntheticFuel,EResourceType::PurifiedWater,EResourceType::Water};
 const int32 Counts[] = {1,1,1,2};
 for (int32 Index = 0; Index < 4; ++Index)
 {
  const FString Path = FString::Printf(TEXT("/Game/Units/Buildings/BlueprintCatalog/%s/BP_%sBuilding.BP_%sBuilding_C"),Keys[Index],Keys[Index],Keys[Index]);
  UClass* Class = LoadClass<AThermalProcessingBuilding>(nullptr,*Path);
  if (!TestNotNull(TEXT("Production blueprint derives from functional class"),Class)) continue;
  const auto* Building = Cast<AThermalProcessingBuilding>(Class->GetDefaultObject());
  TestTrue(TEXT("Configured output matches design"), Building->GetRecipeOutput() == Outputs[Index]);
  TestEqual(TEXT("Configured output count"),Building->OutputPerCycle,Counts[Index]);
  const auto Inputs = Building->GetRecipeInputs();
  auto* Owner = World->SpawnActor<AActor>();
  auto* Production = NewObject<UResourceProductionComponent>(Owner);Production->RegisterComponent();
  Resources->SetResourceAmount(Outputs[Index],0);
  for (const auto& Input : Inputs) Resources->SetResourceAmount(Input.Resource,Input.Cost*3);
  const float Duration = Building->CycleSeconds/3.0f;
  Production->UpdateProduction(Duration,false,Inputs,Outputs[Index],Counts[Index],Duration);
  TestEqual(TEXT("Disabled or unpowered production cannot create items"),Resources->GetResourceAmount(Outputs[Index]),0);
  Production->UpdateProduction(Duration,true,Inputs,Outputs[Index],Counts[Index],Duration);
  TestEqual(TEXT("One cycle creates configured output"),Resources->GetResourceAmount(Outputs[Index]),Counts[Index]);
  for (const auto& Input : Inputs) TestEqual(TEXT("Each input paid exactly"),Resources->GetResourceAmount(Input.Resource),Input.Cost*2);
  if (!Inputs.IsEmpty())
  {
   Resources->SetResourceAmount(Inputs[0].Resource,0);
   Production->UpdateProduction(Duration,true,Inputs,Outputs[Index],Counts[Index],Duration);
   TestEqual(TEXT("Starvation produces nothing"),Resources->GetResourceAmount(Outputs[Index]),Counts[Index]);
   if (Inputs.Num()>1) TestEqual(TEXT("Starvation never spends the other input"),Resources->GetResourceAmount(Inputs[1].Resource),Inputs[1].Cost*2);
  }
  Owner->Destroy();
 }
 World->DestroyWorld(false);
 return true;
}
#endif
