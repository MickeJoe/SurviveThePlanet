#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterShape.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterVariant.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPClusterBuildingPlacementTest,
    "SurviveThePlanet.Placement.ClusterFootprints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPClusterBuildingPlacementTest::RunTest(const FString& Parameters)
{
    UPlanetSectorTemplate* Template = NewObject<UPlanetSectorTemplate>();
    UPlanetTerrainClusterShape* Shape = NewObject<UPlanetTerrainClusterShape>();
    FPlanetSectorClusterSlot Slot;
    Slot.Shape = Shape;
    Template->ClusterSlots.Add(Slot);
    auto Overlaps = [&](double X, double Y, double HalfSize)
    {
        const FVector Corners[] = {{X-HalfSize,Y-HalfSize,0},{X+HalfSize,Y-HalfSize,0},
            {X+HalfSize,Y+HalfSize,0},{X-HalfSize,Y+HalfSize,0}};
        return Template->OverlapsBuildingFootprint(MakeArrayView(Corners), FTransform::Identity);
    };
    Shape->Points = {{-100,-100},{100,-100},{100,100},{-100,100}};
    TestTrue(TEXT("Building inside cluster"), Overlaps(0,0,20));
    TestTrue(TEXT("Building encloses small cluster"), Overlaps(0,0,200));
    TestTrue(TEXT("Only footprint edge overlaps; center outside"), Overlaps(140,0,50));
    TestTrue(TEXT("Touching boundary is blocked"), Overlaps(150,0,50));
    TestFalse(TEXT("Open ground remains buildable"), Overlaps(151,0,50));
    Shape->Points = {{-200,-10},{200,-10},{200,10},{-200,10}};
    TestTrue(TEXT("Crossing edges without contained vertices"), Overlaps(0,0,50));
    Shape->Points = {{-200,-200},{200,-200},{200,200},{100,200},{100,-100},{-100,-100},{-100,200},{-200,200}};
    TestFalse(TEXT("Concave U pocket is not replaced by bounding box"), Overlaps(0,50,40));
    TestTrue(TEXT("U arm blocks placement"), Overlaps(150,50,20));

    Template->ClusterSlots[0].Transform = FTransform(FRotator(0,37,0), FVector(400,-300,0), FVector(-2,1.5,1));
    const FTransform SectorTransform(FRotator(0,-18,0), FVector(6000,8000,500), FVector(1.2,1.2,1));
    const FTransform ShapeWorld = Template->ClusterSlots[0].Transform * SectorTransform;
    TArray<FVector> Transformed;
    for (const FVector& P : TArray<FVector>{{130,30,0},{170,30,0},{170,70,0},{130,70,0}})
        Transformed.Add(ShapeWorld.TransformPosition(P));
    TestTrue(TEXT("Slot and sector rotation, scale and mirroring respected"),
        Template->OverlapsBuildingFootprint(Transformed, SectorTransform));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        MakeUniqueObjectName(nullptr,UWorld::StaticClass()), GetTransientPackage());
    if (!TestNotNull(TEXT("Test world"),World)) return false;
    APlanetSurfaceManager* Surface = World->SpawnActor<APlanetSurfaceManager>();
    APlanetGeneratedSector* Sector = World->SpawnActor<APlanetGeneratedSector>();
    ABaseBuilding* Building = World->SpawnActor<ABaseBuilding>();
    Building->SetPlacementPreview(true);
    Sector->SectorTemplate = Template;
    Template->ClusterSlots[0].Transform = FTransform::Identity;
    // Cell 100 is centered at local origin on the default surface.
    const FVector Center = Surface->GetWorldLocationForCell(FSTPGridCell(100,100));
    Shape->Points = {{Center.X-100,Center.Y-100},{Center.X+100,Center.Y-100},
        {Center.X+100,Center.Y+100},{Center.X-100,Center.Y+100}};
    Sector->SetActorHiddenInGame(true);
    Sector->ClearGenerated();
    TestFalse(TEXT("Unloaded hidden sector still blocks preview"),
        Surface->GetBuildingPlacementForWorldLocation(Center,FIntPoint(1,1)).bValid);
    TestFalse(TEXT("Final building reservation cannot bypass footprint rule"),
        Surface->ReserveCells(Building,FSTPGridCell(100,100),FIntPoint(1,1)));
    TestTrue(TEXT("Building-only rule does not reserve drone grid cells"),
        Surface->CanOccupyCells(FSTPGridCell(100,100),FIntPoint(1,1)));
    TestTrue(TEXT("Open ground can still be reserved"),
        Surface->ReserveCells(Building,FSTPGridCell(110,110),FIntPoint(1,1)));
    Sector->Destroy();
    TestTrue(TEXT("Destroyed sector does not leave stale blockers"),
        Surface->HasBuildingClearance(FSTPGridCell(100,100),FIntPoint(1,1)));
    World->DestroyWorld(false);
    return true;
}
#endif
