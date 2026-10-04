#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Energy/EnergyConnectionRouting.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterShape.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterVariant.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyTerrainRoutingTest, "SurviveThePlanet.Energy.TerrainRouting",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEnergyTerrainRoutingTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 if (!TestNotNull(TEXT("World"),World)) return false;
 auto* Surface = World->SpawnActor<APlanetSurfaceManager>();
 auto* Sector = World->SpawnActor<APlanetGeneratedSector>();
 auto* Shape = NewObject<UPlanetTerrainClusterShape>(Sector);
 auto* Template = NewObject<UPlanetSectorTemplate>(Sector);
 auto* Library = NewObject<UPlanetTerrainClusterLibrary>(Sector);
 auto* Variant = NewObject<UPlanetTerrainClusterVariant>(Sector);
 Variant->CompatibleShape = Shape;
 FPlanetClusterElement Element;
 Element.Mesh = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 Element.Transform = FTransform(FQuat::Identity,FVector::ZeroVector,FVector(5,5,4));
 Variant->Elements.Add(Element);
 Library->Variants.Add(Variant);
 FPlanetSectorClusterSlot Slot;
 Slot.Shape = Shape;
 Slot.Transform = FTransform(FVector(1500,0,0));
 Template->ClusterSlots.Add(Slot);
 Sector->SectorTemplate = Template;
 Sector->VariantLibrary = Library;
 Surface->InvalidateRoutingObstacles();
 const TArray<FBox> LoadedBounds = Surface->GetTerrainRoutingBounds();
 TestEqual(TEXT("One whole nature cluster is an obstacle"),LoadedBounds.Num(),1);
 TArray<FBox2D> Obstacles;
 for (const FBox& Bounds : LoadedBounds)
  Obstacles.Add(FBox2D(FVector2D(Bounds.Min),FVector2D(Bounds.Max)).ExpandBy(125));
 TArray<FVector2D> Path;
 TestTrue(TEXT("Cable detours around nature"),FEnergyConnectionRouting::FindPath(FVector2D(0,0),FVector2D(3000,0),Obstacles,Path));
 TestTrue(TEXT("Nature creates intermediate bends"),Path.Num()>2);
 for (int32 Index=0;Index+1<Path.Num();++Index)
  TestTrue(TEXT("Cable legs remain outside nature"),FEnergyConnectionRouting::IsSegmentClear(Path[Index],Path[Index+1],Obstacles));
 Sector->ClearGenerated();
 Surface->InvalidateRoutingObstacles();
 TestEqual(TEXT("Unloaded nature retains routing obstacles"),Surface->GetTerrainRoutingBounds().Num(),LoadedBounds.Num());
 Sector->Destroy();
 TestEqual(TEXT("Deleted nature invalidates shared obstacle cache"),Surface->GetTerrainRoutingBounds().Num(),0);
 World->DestroyWorld(false);
 return true;
}
#endif
