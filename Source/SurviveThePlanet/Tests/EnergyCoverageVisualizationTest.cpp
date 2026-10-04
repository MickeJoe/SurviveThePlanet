#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "Components/DecalComponent.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Base/BaseModuleSpawnPoint.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "SurviveThePlanetPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnergyCoverageVisualizationTest,
	"SurviveThePlanet.Energy.CoverageVisualization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnergyCoverageVisualizationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("World"), World)) return false;
	UEnergyCoverageSubsystem* System = World->GetSubsystem<UEnergyCoverageSubsystem>();
	ABaseModuleSpawnPoint* SpawnPoint = World->SpawnActor<ABaseModuleSpawnPoint>();
	FindFProperty<FFloatProperty>(ABaseModuleSpawnPoint::StaticClass(), TEXT("EnergyCoverageRadius"))->SetPropertyValue_InContainer(SpawnPoint, 2750.0f);
	AActor* Camp = SpawnPoint->SpawnBaseModule();
	if (!TestNotNull(TEXT("Base Camp"), Camp) || !TestNotNull(TEXT("Coverage system"), System))
	{
		World->DestroyWorld(false);
		return false;
	}
	UEnergyCoverageComponent* Coverage = Camp->FindComponentByClass<UEnergyCoverageComponent>();
	if (!TestNotNull(TEXT("Base Camp coverage component"), Coverage))
	{
		World->DestroyWorld(false);
		return false;
	}
	// The fixture world is not playing, so explicitly perform its registration.
	System->RegisterSource(Coverage);
	TestEqual(TEXT("Base Camp uses configured radius"), Coverage->CoverageRadius, 2750.0f);
	TestFalse(TEXT("Hidden during normal gameplay"), Coverage->IsCoverageVisualizationVisible());
	ASurviveThePlanetPlayerController* Controller = World->SpawnActor<ASurviveThePlanetPlayerController>();
	Controller->SetActiveBuildTool(ESTPBuildTool::EnergyModule);
	TestTrue(TEXT("Selecting a building shows coverage"), Coverage->IsCoverageVisualizationVisible());
	UDecalComponent* Decal = Camp->FindComponentByClass<UDecalComponent>();
	if (TestNotNull(TEXT("Projected decal"), Decal))
	{
		TestEqual(TEXT("Decal radius"), Decal->DecalSize.Y, 2750.0);
		const FVector Center = Decal->GetComponentLocation();
		ABaseBuilding* Ghost = World->SpawnActor<ABaseBuilding>();
		Ghost->SetPlacementPreview(true);
		Ghost->SetActorLocation(FVector(9000, 5000, 100));
		TestTrue(TEXT("Ghost movement leaves coverage stationary"), Decal->GetComponentLocation().Equals(Center));
		Coverage->CoverageRadius = 3100.0f;
		Coverage->RefreshCoverageVisualization();
		TestEqual(TEXT("Radius is tunable without code"), Decal->DecalSize.Y, 3100.0);
		Ghost->Destroy();
	}
	Controller->SetActiveBuildTool(ESTPBuildTool::None);
	TestFalse(TEXT("Leaving placement hides coverage"), Coverage->IsCoverageVisualizationVisible());
	Controller->SetActiveBuildTool(ESTPBuildTool::WaterCollector);
	TestTrue(TEXT("Starting placement again shows coverage"), Coverage->IsCoverageVisualizationVisible());

	AActor* OtherSource = World->SpawnActor<AActor>();
	UEnergyCoverageComponent* OtherCoverage = NewObject<UEnergyCoverageComponent>(OtherSource);
	OtherSource->AddInstanceComponent(OtherCoverage);
	OtherCoverage->RegisterComponent();
	System->RegisterSource(OtherCoverage);
	TestTrue(TEXT("New sources inherit current visualization state"), OtherCoverage->IsCoverageVisualizationVisible());
	Controller->SetActiveBuildTool(ESTPBuildTool::None);
	TestFalse(TEXT("Cancel hides Base Camp coverage"), Coverage->IsCoverageVisualizationVisible());
	TestFalse(TEXT("Cancel hides all sources"), OtherCoverage->IsCoverageVisualizationVisible());
	System->UnregisterSource(OtherCoverage);
	OtherSource->Destroy();
	System->SetCoverageVisualizationVisible(true);
	TestTrue(TEXT("Registry remains usable after source removal"), Coverage->IsCoverageVisualizationVisible());
	Coverage->CoverageRadius = 0;
	System->SetCoverageVisualizationVisible(true);
	TestFalse(TEXT("Zero radius hides coverage"), Coverage->IsCoverageVisualizationVisible());
	Controller->Destroy();
	System->UnregisterSource(Coverage);
	Camp->Destroy();
	World->DestroyWorld(false);
	return true;
}
#endif
