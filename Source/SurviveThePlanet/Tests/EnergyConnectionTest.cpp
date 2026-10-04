#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyConnectionTest,"SurviveThePlanet.Energy.ConnectionPreview",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnergyConnectionTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 if(!TestNotNull(TEXT("World"),World))return false;
 auto* System=World->GetSubsystem<UEnergyCoverageSubsystem>();
 auto MakeSource=[&](FVector Position)
 {
  auto* Building=World->SpawnActor<ABaseBuilding>(ABaseBuilding::StaticClass(),Position,FRotator::ZeroRotator);
  Building->SetConstructionProgress(1);
  auto* Source=NewObject<UEnergyCoverageComponent>(Building);Source->SetupAttachment(Building->GetRootComponent());Source->RegisterComponent();System->RegisterSource(Source);
  return Source;
 };
 auto* Camp=MakeSource(FVector(0,0,0));auto* Other=MakeSource(FVector(2000,0,0));
 TestEqual(TEXT("Nearest completed source"),System->FindNearestSource(FVector(1900,0,0),nullptr),Other);
 TestEqual(TEXT("Switch margin prevents midpoint flicker"),System->FindNearestSource(FVector(1050,0,0),nullptr,Camp,150),Camp);
 TestEqual(TEXT("Clear distance advantage changes parent"),System->FindNearestSource(FVector(1200,0,0),nullptr,Camp,150),Other);
 Cast<ABaseBuilding>(Other->GetOwner())->SetConstructionProgress(0);
 TestEqual(TEXT("Unfinished sources excluded"),System->FindNearestSource(FVector(1900,0,0),nullptr),Camp);
 auto* Manager=World->GetSubsystem<UBuildingManagerSubsystem>();
 auto* Ghost=World->SpawnActor<ABaseBuilding>(Manager->GetBuildingClass(ESTPBuildTool::EnergyExtender),FVector(3000,0,186.55),FRotator::ZeroRotator);
 Ghost->SetPlacementPreview(true);
 auto* Connection=NewObject<UEnergyConnectionComponent>(Ghost);Connection->SetupAttachment(Ghost->GetRootComponent());Connection->RegisterComponent();
 Connection->UpdateRoute(true,true);
 TestEqual(TEXT("Preview connects outside existing coverage"),Connection->GetSourceActor(),Camp->GetOwner());
 TestTrue(TEXT("Intermediate evenly spaced poles created"),Connection->GetPoleCount()>0);
 const int32 Rebuilds=Connection->GetRouteRebuildCount();
 const int32 ComponentCount=Ghost->GetComponents().Num();
 Connection->UpdateRoute(true,true);
 TestEqual(TEXT("Stationary preview does not rebuild"),Connection->GetRouteRebuildCount(),Rebuilds);
 Connection->UpdateRoute(true,false);
 TestEqual(TEXT("Validity recolor does not rebuild"),Connection->GetRouteRebuildCount(),Rebuilds);
 Ghost->SetActorLocation(FVector(2400,1200,186.55));Connection->UpdateRoute(true,true);
 TestEqual(TEXT("Moving route reuses components"),Ghost->GetComponents().Num(),ComponentCount);
 TArray<UPrimitiveComponent*> Primitives;Ghost->GetComponents(Primitives);
 for(auto* Primitive:Primitives)
 {
  if(Cast<USplineMeshComponent>(Primitive)||Cast<UInstancedStaticMeshComponent>(Primitive))
  {
   TestEqual(TEXT("Connection has no collision"),Primitive->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
   TestFalse(TEXT("Connection does not affect navigation"),Primitive->CanEverAffectNavigation());
   TestFalse(TEXT("Coverage decal cannot recolor connection"),Primitive->bReceivesDecals);
   if (auto* Cable = Cast<USplineMeshComponent>(Primitive))
   {
    if (Cable->IsVisible())
    {
     const FBox RenderBounds = Cable->Bounds.GetBox().ExpandBy(1);
     TestTrue(TEXT("Moved cable render bounds contain start"),RenderBounds.IsInsideOrOn(Cable->GetStartPosition()));
     TestTrue(TEXT("Moved cable render bounds contain end"),RenderBounds.IsInsideOrOn(Cable->GetEndPosition()));
    }
   }
   if (auto* Poles = Cast<UInstancedStaticMeshComponent>(Primitive))
    TestEqual(TEXT("Preview preserves authored ivory material"),Poles->GetMaterial(1),Poles->GetStaticMesh()->GetMaterial(1));
  }
 }
 auto* Placed=World->SpawnActor<ABaseBuilding>(Manager->GetBuildingClass(ESTPBuildTool::EnergyExtender),Ghost->GetActorLocation(),FRotator::ZeroRotator);
 auto* Final=NewObject<UEnergyConnectionComponent>(Placed);Final->SetupAttachment(Placed->GetRootComponent());Final->RegisterComponent();
 Final->CopyPlacedRoute(Connection);
 TestEqual(TEXT("Placement preserves selected parent"),Final->GetSourceActor(),Connection->GetSourceActor());
 TestEqual(TEXT("Placement preserves pole count"),Final->GetPoleCount(),Connection->GetPoleCount());
 auto* Surface = World->SpawnActor<APlanetSurfaceManager>();
 Connection->UpdateRoute(true,true);
 FSTPGridCell ObstacleCell;
 Surface->GetCellForWorldLocation(FVector(1200,600,0), ObstacleCell);
 auto* Obstacle = World->SpawnActor<ABaseBuilding>();
 Obstacle->SetActorLocation(Surface->GetWorldLocationForCell(ObstacleCell));
 const int32 BeforeObstacle = Connection->GetRouteRebuildCount();
 TestTrue(TEXT("A building can reserve cells through decorative cables"), Surface->ReserveCells(Obstacle,ObstacleCell,FIntPoint(3,3)));
 TestTrue(TEXT("Reservation automatically reroutes a stationary connection"), Connection->GetRouteRebuildCount() > BeforeObstacle);
 const int32 BeforeRemoval = Connection->GetRouteRebuildCount();
 Surface->ReleaseCells(Obstacle);
 TestTrue(TEXT("Removal automatically refreshes route"), Connection->GetRouteRebuildCount() > BeforeRemoval);
 Obstacle->Destroy();
 Ghost->Destroy();Placed->Destroy();Camp->GetOwner()->Destroy();Other->GetOwner()->Destroy();
 World->DestroyWorld(false);return true;
}
#endif
