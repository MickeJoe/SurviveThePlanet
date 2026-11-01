#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/UraniumResourceSource.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Buildings/CrudeWorkshop.h"
#include "Gameplay/Drones/ExplorerDrone.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FColonySaveRoundTripTest, "SurviveThePlanet.SaveLoad.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FColonySaveRoundTripTest::RunTest(const FString& Parameters)
{
	auto* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone(MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	UWorld* World = Instance->GetWorld(); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->GetWorldSettings()->NotifyBeginPlay();
	auto* Saves = Instance->GetSubsystem<UColonySaveSubsystem>();
	auto* Resources = World->SpawnActor<AResourceManager>();
	Resources->SetResourceAmount(EResourceType::Steel, 73); Resources->SetCredits(4321);
	auto* Source = World->SpawnActor<AUraniumResourceSource>(); Source->ExtractResource(17);
	const int32 Remaining = Source->GetRemainingAmount();
	auto* Building = World->SpawnActor<ACargoBay>(FVector(2000,2000,0), FRotator::ZeroRotator);
	Building->SetConstructionProgress(0.42f);
	auto* Objectives = World->GetSubsystem<UObjectiveSubsystem>();
	Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
	auto* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>(); Mission->SetMissionConfidence(53);
	auto* Grid = World->SpawnActor<AHexSectorGrid>();
	auto* Explorer = World->SpawnActor<AExplorerDrone>(FVector(1500,1500,250), FRotator::ZeroRotator);
	TestTrue(TEXT("Explorer starts a mission before saving"), Explorer->ActivateExploration());
	auto* Workshop = World->SpawnActor<ACrudeWorkshop>(FVector(5000,5000,0), FRotator::ZeroRotator);
	Workshop->SetConstructionProgress(1.0f);
	UClass* WorkerClass = LoadClass<ABaseDrone>(nullptr,TEXT("/Game/BluePrints/WorkingDrone/BP_ConstructionDrone.BP_ConstructionDrone_C"));
	auto* Worker = World->SpawnActor<ABaseDrone>(WorkerClass,FVector(5200,5200,0),FRotator::ZeroRotator);
	TestTrue(TEXT("Worker assigned before saving"), Worker && Worker->AssignToBuilding(Workshop,0));
	int32 DroneCount = 0; for (TActorIterator<ABaseDrone> It(World); It; ++It) ++DroneCount;
	FText Status;
	UColonySaveGame* Save = Saves->CaptureWorld(World, Status);
	if (TestNotNull(TEXT("Snapshot captured"), Save))
	{
		TArray<uint8> Bytes;
		TestTrue(TEXT("Snapshot serializes to a portable save file"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
		auto* Loaded = Cast<UColonySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
		Resources->SetResourceAmount(EResourceType::Steel, 1); Resources->SetCredits(0);
		Mission->SetMissionConfidence(5); Source->ExtractResource(20);
		Building->SetActorLocation(FVector(9000,9000,0));
		World->SpawnActor<ABaseDrone>(FVector(7000,7000,0), FRotator::ZeroRotator);
		TestTrue(TEXT("Saved world restores"), Saves->RestoreWorld(World, Loaded, Status));
		TestEqual(TEXT("Inventory restored"), Resources->GetResourceAmount(EResourceType::Steel), 73);
		TestEqual(TEXT("Credits restored"), Resources->GetCredits(), 4321);
		Mission->Tick(0.0f);
		TestEqual(TEXT("Confidence restored without granting rewards"), Mission->GetMissionConfidence(), 53.0f);
		TestEqual(TEXT("Deposit depletion restored"), Source->GetRemainingAmount(), Remaining);
		ACargoBay* RestoredBuilding = nullptr;
		for (TActorIterator<ACargoBay> It(World); It; ++It) if (!It->IsActorBeingDestroyed()) RestoredBuilding = *It;
		if (TestNotNull(TEXT("Building recreated at saved location"), RestoredBuilding))
		{
			TestTrue(TEXT("Construction progress restored"), FMath::IsNearlyEqual(RestoredBuilding->GetConstructionProgress(), 0.42f));
			TestTrue(TEXT("Building transform restored"), RestoredBuilding->GetActorLocation().Equals(FVector(2000,2000,0)));
		}
		AExplorerDrone* RestoredExplorer = nullptr;
		for (TActorIterator<AExplorerDrone> It(World); It; ++It) if (!It->IsActorBeingDestroyed()) RestoredExplorer = *It;
		if (TestNotNull(TEXT("Explorer restored"), RestoredExplorer))
		{
			TestTrue(TEXT("Exploration mission retained"), RestoredExplorer->IsExploring());
			const FVector Before = RestoredExplorer->GetActorLocation(); RestoredExplorer->Tick(0.1f);
			TestFalse(TEXT("Explorer resumes flying after restore"), RestoredExplorer->GetActorLocation().Equals(Before));
		}
		TestTrue(TEXT("Worker building reference restored"), Worker->GetAssignedBuilding()==Workshop);
		TestEqual(TEXT("Worker slot restored"), Worker->GetAssignedBuildingSlot(), 0);
		Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
		int32 RestoredDrones = 0; for (TActorIterator<ABaseDrone> It(World); It; ++It) if (!It->IsActorBeingDestroyed()) ++RestoredDrones;
		TestEqual(TEXT("Drones restored and completed objective rewards never duplicated"), RestoredDrones, DroneCount);
		TestFalse(TEXT("Invalid slot rejected without changing the colony"), Saves->LoadColony(TEXT("../bad"), Status));
		TestEqual(TEXT("Rejected load preserves inventory"), Resources->GetResourceAmount(EResourceType::Steel), 73);
	}
	const FString SaveDisplayName = TEXT("Save test ") + FGuid::NewGuid().ToString();
	TestTrue(TEXT("First named save succeeds"), Saves->SaveColony(SaveDisplayName, Status));
	TestTrue(TEXT("Second identically named save succeeds"), Saves->SaveColony(SaveDisplayName, Status));
	int32 MatchingSlots = 0; TSet<FString> SlotIds;
	for (const auto& Entry : Saves->ListSaves()) if (Entry.DisplayName == SaveDisplayName)
	{
		++MatchingSlots; SlotIds.Add(Entry.Slot);
		UGameplayStatics::DeleteGameInSlot(Entry.Slot, 0);
	}
	TestEqual(TEXT("Multiple slots may use the same display name"), MatchingSlots, 2);
	TestEqual(TEXT("Same-name saves never overwrite one another"), SlotIds.Num(), 2);
	Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
