#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterShape.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterVariant.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPMineTerrainBoundsTest,
	"SurviveThePlanet.Placement.MineTerrainBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPMineTerrainBoundsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("World"), World)) return false;
	APlanetSurfaceManager* Surface = World->SpawnActor<APlanetSurfaceManager>();
	APlanetGeneratedSector* Sector = World->SpawnActor<APlanetGeneratedSector>();
	UPlanetSectorTemplate* Template = NewObject<UPlanetSectorTemplate>();
	UPlanetTerrainClusterShape* Shape = NewObject<UPlanetTerrainClusterShape>();
	Shape->Points = {{-10,-10},{10,-10},{10,10},{-10,10}};
	FPlanetSectorClusterSlot Slot;
	Slot.Shape = Shape;
	Slot.Transform = FTransform(FRotator(0, 37, 0), FVector(500, 300, 0), FVector(-1.2, 1.5, 1));
	Template->ClusterSlots.Add(Slot);
	UPlanetTerrainClusterVariant* Variant = NewObject<UPlanetTerrainClusterVariant>();
	Variant->CompatibleShape = Shape;
	FPlanetClusterElement Element;
	Element.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Cube mesh"), Element.Mesh.Get())) { World->DestroyWorld(false); return false; }
	// Intentionally outside the tiny authored contour, rotated and nonuniformly scaled.
	Element.Transform = FTransform(FRotator(0, 23, 0), FVector(800, 0, 0), FVector(2, 4, 3));
	Variant->Elements.Add(Element);
	UPlanetTerrainClusterLibrary* Library = NewObject<UPlanetTerrainClusterLibrary>();
	Library->Variants.Add(Variant);
	Sector->SectorTemplate = Template;
	Sector->VariantLibrary = Library;
	Sector->SetActorTransform(FTransform(FRotator(0, -18, 0), FVector(500, -400, 0), FVector(1.2, 1.2, 1)));
	const FTransform MeshWorld = (Element.Transform * Slot.Transform) * Sector->GetActorTransform();
	const FSTPGridPlacement Placement = Surface->GetPlacementForWorldLocation(MeshWorld.GetLocation(), FIntPoint(1,1));
	TestFalse(TEXT("Mesh outside contour blocks before any ISMs are loaded"),
		Surface->HasTerrainClearance(Placement.OriginCell, FIntPoint(1,1)));
	Sector->Generate();
	TestFalse(TEXT("Resident mesh blocks"), Surface->HasTerrainClearance(Placement.OriginCell, FIntPoint(1,1)));
	Sector->ClearGenerated();
	TestFalse(TEXT("Unload keeps selected mesh footprint"), Surface->HasTerrainClearance(Placement.OriginCell, FIntPoint(1,1)));
	TestTrue(TEXT("Open terrain stays buildable"), Surface->HasTerrainClearance(FSTPGridCell(130,130), FIntPoint(1,1), 3));
	// Search the edge so the test is independent of transformed mesh-bound rounding.
	bool bFoundMarginCase = false;
	for (int32 X = Placement.OriginCell.X - 15; X <= Placement.OriginCell.X + 15 && !bFoundMarginCase; ++X)
	{
		const FSTPGridCell Cell(X, Placement.OriginCell.Y);
		if (Surface->HasTerrainClearance(Cell, FIntPoint(1,1), 0)
			&& !Surface->HasTerrainClearance(Cell, FIntPoint(1,1), 3))
		{
			bFoundMarginCase = true;
		}
	}
	TestTrue(TEXT("Extra margin excludes visually adjacent cells"), bFoundMarginCase);
	Sector->Destroy();
	TestTrue(TEXT("Destroyed sector no longer blocks"), Surface->HasTerrainClearance(Placement.OriginCell, FIntPoint(1,1), 3));
	World->DestroyWorld(false);
	return true;
}
#endif
