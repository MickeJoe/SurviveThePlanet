#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/DecalComponent.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "SurviveThePlanetPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyExtenderBuildTest,
	"SurviveThePlanet.Energy.ExtenderBuilding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnergyExtenderBuildTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("World"), World)) return false;
	UBuildingManagerSubsystem* Manager = World->GetSubsystem<UBuildingManagerSubsystem>();
	UBuildingDataAsset* Definition = Manager->GetDefinition(ESTPBuildTool::EnergyExtender);
	if (!TestNotNull(TEXT("Extender registered in building catalog"), Definition))
	{
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Energy menu category"), Definition->BuildCategory, ESTPBuildCategory::Energy);
	TestTrue(TEXT("Visible building button"), Definition->bShowInBuildToolbar);
	TestTrue(TEXT("Initially owned"), Definition->bBlueprintInitiallyOwned);
	TestNotNull(TEXT("Authored button icon"), Definition->ToolbarIcon.Get());
	TestNotNull(TEXT("Imported Blender mesh"), Definition->BuildingMesh.Get());
	ABaseBuilding* Extender = World->SpawnActor<ABaseBuilding>(Manager->GetBuildingClass(ESTPBuildTool::EnergyExtender), FVector(6000,0,0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Spawn extender outside initial 20 metre coverage"), Extender))
	{
		World->DestroyWorld(false);
		return false;
	}
	UEnergyCoverageComponent* Coverage = Extender->FindComponentByClass<UEnergyCoverageComponent>();
	if (!TestNotNull(TEXT("Reusable coverage source"), Coverage))
	{
		World->DestroyWorld(false);
		return false;
	}
	TestEqual(TEXT("Mesh fits three metre grid footprint"), Extender->GetGridFootprint(), FIntPoint(3,3));
	TestEqual(TEXT("Extender radius"), Coverage->CoverageRadius, 2000.0f);
	UEnergyCoverageSubsystem* System = World->GetSubsystem<UEnergyCoverageSubsystem>();
 auto* Camp = World->SpawnActor<ABaseBuilding>(ABaseBuilding::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
 Camp->SetConstructionProgress(1);
 auto* CampCoverage = NewObject<UEnergyCoverageComponent>(Camp);
 CampCoverage->SetupAttachment(Camp->GetRootComponent());
 CampCoverage->RegisterComponent();
 System->RegisterSource(CampCoverage);
 auto* Connection = NewObject<UEnergyConnectionComponent>(Extender);
 Connection->SetupAttachment(Extender->GetRootComponent());
 Connection->RegisterComponent();
 Connection->UpdateRoute(false,true);
 System->RegisterSource(Coverage);
	ASurviveThePlanetPlayerController* Controller = World->SpawnActor<ASurviveThePlanetPlayerController>();
	Controller->SetActiveBuildTool(ESTPBuildTool::EnergyExtender);
	TestTrue(TEXT("Completed source shows coverage while placing"), Coverage->IsCoverageVisualizationVisible());
	Extender->SetConstructionProgress(0);
	TestFalse(TEXT("Unfinished extender supplies no coverage"), Coverage->IsCoverageVisualizationVisible());
	Extender->SetConstructionProgress(0.5f);
	TestFalse(TEXT("Partial construction supplies no coverage"), Coverage->IsCoverageVisualizationVisible());
	Extender->SetConstructionProgress(1);
	TestTrue(TEXT("Completing outside existing area activates coverage"), Coverage->IsCoverageVisualizationVisible());
	Extender->SetPlacementPreview(true);
	TestFalse(TEXT("Ghost has no energy coverage"), Coverage->IsCoverageVisualizationVisible());
	Controller->SetActiveBuildTool(ESTPBuildTool::None);
	System->UnregisterSource(Coverage);
	Extender->Destroy();
	System->UnregisterSource(CampCoverage);
	Camp->Destroy();
	Controller->Destroy();
	World->DestroyWorld(false);
	return true;
}
#endif
