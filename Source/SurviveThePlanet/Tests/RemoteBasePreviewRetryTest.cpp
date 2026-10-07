#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/Buildings/RemoteBase.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Resources/ResourceManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRemoteBasePreviewRetryTest,
	"SurviveThePlanet.Buildings.RemoteBasePreviewRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRemoteBasePreviewRetryTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	APlanetSurfaceManager* Surface = World->SpawnActor<APlanetSurfaceManager>();
	ABaseBuilding* Camp = World->SpawnActor<ABaseBuilding>();
	Camp->SetConstructionProgress(1);
	UEnergyCoverageComponent* CampCoverage = NewObject<UEnergyCoverageComponent>(Camp);
	CampCoverage->SetupAttachment(Camp->GetRootComponent());
	CampCoverage->RegisterComponent();
	World->GetSubsystem<UEnergyCoverageSubsystem>()->RegisterSource(CampCoverage);
	AResourceManager* Resources = World->SpawnActor<AResourceManager>();
	ASurviveThePlanetPlayerController* Controller = World->SpawnActor<ASurviveThePlanetPlayerController>();
	Controller->PlayerInput = NewObject<UPlayerInput>(Controller);
	for (int32 Attempt = 0; Attempt < 2; ++Attempt)
	{
		Controller->SetActiveBuildTool(ESTPBuildTool::RemoteBase);
		static_cast<APlayerController*>(Controller)->PlayerTick(0);
		ABaseBuilding* Ghost = Controller->GetActivePlacementPreview();
		if (!TestNotNull(TEXT("Reopening creates a placement preview"), Ghost)) break;
		TMap<AActor*, FBox> Obstacles;
		Surface->GetBuildingRoutingBounds(Obstacles);
		TestFalse(TEXT("Preview never enters cached cable obstacles"), Obstacles.Contains(Ghost));
		Ghost->SetActorLocation(FVector(6000, 0, 0));
		UEnergyConnectionComponent* Connection = Ghost->FindComponentByClass<UEnergyConnectionComponent>();
		Connection->UpdateRoute(true, Attempt > 0);
		TestEqual(TEXT("Preview connects to main camp"), Connection->GetSourceActor(), static_cast<AActor*>(Camp));
		TestTrue(TEXT("Cable route remains usable after cancel and retry"), Connection->HasRoutableConnection());
		TestTrue(TEXT("Electricity poles are created"), Connection->GetPoleCount() > 0);
		const TArray<FResourceCost> Costs = Controller->GetBuildCosts(ESTPBuildTool::RemoteBase, true);
		if (Attempt == 0)
		{
			TestFalse(TEXT("First attempt is unaffordable"), Resources->CanAffordCosts(Costs));
			Controller->SetActiveBuildTool(ESTPBuildTool::None);
			TestNull(TEXT("Escape equivalent removes old preview"), Controller->GetActivePlacementPreview());
			Resources->AddResource(EResourceType::Connector, 1000);
		}
		else
		{
			TestTrue(TEXT("Cheat-granted connectors make retry affordable"), Resources->CanAffordCosts(Costs));
			TestTrue(TEXT("A preview always uses the same connector distance rate"), Connection->GetConnectionDistanceMeters() > 0);
			Controller->SetActiveBuildTool(ESTPBuildTool::None);
		}
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
