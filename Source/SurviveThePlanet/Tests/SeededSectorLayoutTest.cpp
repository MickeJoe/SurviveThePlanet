#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Gameplay/World/HexSectorGrid.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPSeededSectorLayoutTest,
	"SurviveThePlanet.PlanetGeneration.SeededSectorLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	FString LayoutSignature(const AHexSectorGrid* Grid)
	{
		FString Result;
		for (const FHexSector& Sector : Grid->Sectors)
		{
			Result += FString::Printf(TEXT("%d:%s:%d:%d|"), Sector.Id, *Sector.TemplateId.ToString(),
				Sector.TemplateRotationDegrees, Sector.bTemplateMirrored ? 1 : 0);
		}
		return Result;
	}
}

bool FSTPSeededSectorLayoutTest::RunTest(const FString& Parameters)
{
	AHexSectorGrid* First = NewObject<AHexSectorGrid>();
	AHexSectorGrid* Same = NewObject<AHexSectorGrid>();
	AHexSectorGrid* Different = NewObject<AHexSectorGrid>();
	First->GridRadius = Same->GridRadius = Different->GridRadius = 1;
	First->LayoutSeed = Same->LayoutSeed = 4815;
	Different->LayoutSeed = 4816;
	First->RebuildGrid();
	Same->RebuildGrid();
	Different->RebuildGrid();

	TestEqual(TEXT("Radius-one planet contains seven sectors"), First->Sectors.Num(), 7);
	TestTrue(TEXT("At least six authored templates exist"), First->SectorTemplates.Num() >= 6);
	TSet<FName> UsedTemplates;
	for (const FHexSector& Sector : First->Sectors) UsedTemplates.Add(Sector.TemplateId);
	TestTrue(TEXT("Small planet uses at least six templates"), UsedTemplates.Num() >= 6);
	TestEqual(TEXT("Same seed produces identical layout"), LayoutSignature(First), LayoutSignature(Same));
	TestNotEqual(TEXT("Different seed produces a different layout"), LayoutSignature(First), LayoutSignature(Different));

	FSectorTemplateDefinition HQTemplate;
	TestTrue(TEXT("Starting template resolves"), First->GetTemplateForSector(First->StartingSectorId, HQTemplate));
	TestTrue(TEXT("Starting sector supports HQ"), HQTemplate.bSupportsHQ);
	TestTrue(TEXT("Starting sector has buildable room"), !HQTemplate.BuildablePockets.IsEmpty());
	FString Diagnostic;
	TestTrue(TEXT("Generated layout validates"), First->ValidateGeneratedLayout(Diagnostic));
	return true;
}
#endif
