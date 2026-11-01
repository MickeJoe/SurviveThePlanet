#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSavedConnectionTest, "SurviveThePlanet.SaveLoad.RuntimePowerConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSavedConnectionTest::RunTest(const FString& Parameters)
{
	auto* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone(MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->GetWorldSettings()->NotifyBeginPlay();
	World->SpawnActor<AResourceManager>();
	auto* Headquarters = World->SpawnActor<ABaseBuilding>();
	Headquarters->SetConstructionProgress(1);
	auto* Coverage = NewObject<UEnergyCoverageComponent>(Headquarters,TEXT("SavedHeadquartersCoverage"));
	Coverage->SetupAttachment(Headquarters->GetRootComponent()); Headquarters->AddInstanceComponent(Coverage); Coverage->RegisterComponent();
	auto* Manager = World->GetSubsystem<UBuildingManagerSubsystem>();
	auto* Extender = World->SpawnActor<ABaseBuilding>(Manager->GetBuildingClass(ESTPBuildTool::EnergyExtender),FVector(2800,0,0),FRotator::ZeroRotator);
	Extender->SetConstructionProgress(1);
	auto* Connection = NewObject<UEnergyConnectionComponent>(Extender,TEXT("PlacedRuntimeConnection"));
	Connection->SetupAttachment(Extender->GetRootComponent()); Extender->AddInstanceComponent(Connection); Connection->RegisterComponent();
	Connection->RestorePlacedRoute();
	TestTrue(TEXT("Placed connection has cable spans before saving"),Connection->HasUsableConnection());
	const int32 OriginalPoles = Connection->GetPoleCount();
	auto* Saves = Instance->GetSubsystem<UColonySaveSubsystem>();
	FText Status;
	auto* Save = Saves->CaptureWorld(World,Status);
	if (TestNotNull(TEXT("Snapshot"),Save))
	{
		// Simulate the first released format, which saved only Source on runtime connections.
		for (auto& Record : Save->Systems) if (Record.Id.EndsWith(TEXT("/PlacedRuntimeConnection")))
			Record.Fields.RemoveAll([](const auto& Field) { return Field.Name != TEXT("Source"); });
		TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Save,Bytes);
		auto* Loaded = Cast<UColonySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
		Extender->Destroy(); Headquarters->Destroy();
		TestTrue(TEXT("Legacy runtime-component snapshot restores"),Saves->RestoreWorld(World,Loaded,Status));
		UEnergyConnectionComponent* Restored = nullptr;
		for (TActorIterator<ABaseBuilding> It(World); It; ++It)
			if (!It->IsActorBeingDestroyed()) if (auto* Candidate=It->FindComponentByClass<UEnergyConnectionComponent>()) Restored=Candidate;
		if (TestNotNull(TEXT("Missing instance component recreated"),Restored))
		{
			TestTrue(TEXT("Restored connection has usable cable spans"),Restored->HasUsableConnection());
			TestEqual(TEXT("Intermediate poles reconstructed"),Restored->GetPoleCount(),OriginalPoles);
			TestNotNull(TEXT("Saved power source resolves to restored actor"),Restored->GetSourceActor());
			TestTrue(TEXT("Stable saved component name retained"),Restored->GetFName()==TEXT("PlacedRuntimeConnection"));
		}
	}
	Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
