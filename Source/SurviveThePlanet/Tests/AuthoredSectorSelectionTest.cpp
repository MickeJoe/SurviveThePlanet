#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPAuthoredSectorSelectionTest,
    "SurviveThePlanet.PlanetGeneration.AuthoredSectorSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPAuthoredSectorSelectionTest::RunTest(const FString& Parameters)
{
    TArray<UPlanetSectorTemplate*> Catalog;
    for (int32 Index = 0; Index < 7; ++Index)
    {
        UPlanetSectorTemplate* Template = NewObject<UPlanetSectorTemplate>();
        TestFalse(TEXT("New templates require explicit starting-sector opt-in"), Template->bCanBeStartingSector);
        Template->bCanBeStartingSector = Index < 3;
        Catalog.Add(Template);
    }
    TArray<UPlanetSectorTemplate*> Reordered;
    for (int32 Index = 6; Index >= 0; --Index) Reordered.Add(Catalog[Index]);
    Reordered.Add(nullptr);
    Reordered.Add(Catalog[0]);
    TSet<UPlanetSectorTemplate*> Used;
    for (int32 Seed = 0; Seed < 1000; ++Seed)
    {
        UPlanetSectorTemplate* Start = UPlanetSectorTemplate::SelectForSector(Catalog, Seed, 12, true);
        TestTrue(TEXT("Nonzero starting sector only selects eligible templates"), Start && Start->bCanBeStartingSector);
        UPlanetSectorTemplate* Ordinary = UPlanetSectorTemplate::SelectForSector(Catalog, Seed, 5, false);
        Used.Add(Ordinary);
        TestTrue(TEXT("Catalog order, nulls and duplicates do not affect selection"),
            Ordinary == UPlanetSectorTemplate::SelectForSector(Reordered, Seed, 5, false));
    }
    TestEqual(TEXT("All seven templates can appear in ordinary sectors"), Used.Num(), 7);
    for (UPlanetSectorTemplate* Template : Catalog) Template->bCanBeStartingSector = false;
    TestNull(TEXT("No unsafe fallback when no start is eligible"), UPlanetSectorTemplate::SelectForSector(Catalog, 1, 0, true));
    TestNull(TEXT("Empty catalog is safe"), UPlanetSectorTemplate::SelectForSector({}, 1, 0, false));
    return true;
}
#endif
