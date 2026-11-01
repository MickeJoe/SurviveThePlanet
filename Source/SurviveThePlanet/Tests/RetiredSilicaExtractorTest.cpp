#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Serialization/MemoryWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRetiredSilicaExtractorTest, "SurviveThePlanet.SaveLoad.RetiredStandaloneSilica",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRetiredSilicaExtractorTest::RunTest(const FString& Parameters)
{
	auto* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone(MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->GetWorldSettings()->NotifyBeginPlay();
	auto* Resources = World->SpawnActor<AResourceManager>();
	Resources->SetResourceAmount(EResourceType::Iron,17); Resources->SetResourceAmount(EResourceType::Silica,123);
	auto* Manager = World->GetSubsystem<UBuildingManagerSubsystem>();
	Instance->GetSubsystem<UBuildingBlueprintSubsystem>()->GrantBlueprint(ESTPBuildTool::SilicaExtractor);
	TestFalse(TEXT("Silica has no standalone placement tool"),Manager->IsStandalonePlacementEnabled(ESTPBuildTool::SilicaExtractor));
	TestFalse(TEXT("Standalone extractor excluded from toolbar"),Manager->GetToolbarDefinitions().ContainsByPredicate([](const auto* Definition) { return Definition->BuildTool==ESTPBuildTool::SilicaExtractor; }));
	TestTrue(TEXT("Silica extraction unlock remains owned"),Instance->GetSubsystem<UBuildingBlueprintSubsystem>()->OwnsBlueprint(ESTPBuildTool::SilicaExtractor));
	auto* Retired = World->SpawnActor<ABaseBuilding>(Manager->GetBuildingClass(ESTPBuildTool::SilicaExtractor),FVector(3000,3000,0),FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Legacy class available for recognizing old saves"),Retired)) return false;
	Retired->SetConstructionProgress(1);
	TestTrue(TEXT("Legacy standalone actor identified"),Manager->IsRetiredStandaloneBuilding(Retired));
	UClass* DepositClass=LoadClass<ABaseResourceSource>(nullptr,TEXT("/Game/Resources/SurfaceDeposits/BP_SilicaDeposit.BP_SilicaDeposit_C"));
	auto* Deposit=World->SpawnActor<ABaseResourceSource>(DepositClass,FVector(0,0,0),FRotator::ZeroRotator);
	auto* Mine=World->SpawnActor<AMiningMachine>(Deposit->GetMineBlueprint(),Deposit->GetActorLocation(),FRotator::ZeroRotator);
	// This isolated world has no terrain grid; reserve the source to establish
	// the same ownership that successful Build Mine placement establishes.
	TestTrue(TEXT("Deposit reserves real mine"),Deposit->TryReserveMiningMachine(Mine));
	TestTrue(TEXT("Existing Build Mine class accepts silica deposit"),Mine->AttachToResourceSource(Deposit));
	Mine->SetConstructionProgress(1);
	TestFalse(TEXT("Deposit-bound Silica Extractor is not retired"),Manager->IsRetiredStandaloneBuilding(Mine));
	UClass* WorkerClass=LoadClass<ABaseDrone>(nullptr,TEXT("/Game/BluePrints/WorkingDrone/BP_ConstructionDrone.BP_ConstructionDrone_C"));
	auto* Worker=World->SpawnActor<ABaseDrone>(WorkerClass,FVector(3200,3000,0),FRotator::ZeroRotator);
	TestTrue(TEXT("Legacy assignment prepared"),Worker->AssignToBuilding(Retired,0));
	auto* Saves=Instance->GetSubsystem<UColonySaveSubsystem>(); FText Status;
	auto* Save=Saves->CaptureWorld(World,Status);
	TestFalse(TEXT("New snapshots omit retired standalone actors"),Save->Actors.ContainsByPredicate([&](const auto& Record) { return Record.Id==Retired->GetName(); }));
	// Recreate an old snapshot containing the retired actor, its component and a worker reference.
	auto& Legacy=Save->Actors.AddDefaulted_GetRef(); Legacy.Id=Retired->GetName(); Legacy.Class=FSoftClassPath(Retired->GetClass()); Legacy.Transform=Retired->GetActorTransform();
	const FString LegacyId=Legacy.Id;
	auto& Component=Save->Systems.AddDefaulted_GetRef(); Component.Id=LegacyId+TEXT("/OldConnection"); Component.Class=FSoftClassPath(UEnergyConnectionComponent::StaticClass());
	for (auto& Record : Save->Actors) if (Record.Id==Worker->GetName())
		for (auto& Field : Record.Fields) if (Field.Name==TEXT("AssignedBuilding"))
		{
			Field.Data.Reset(); FMemoryWriter Writer(Field.Data,true); FString Reference=TEXT("@")+LegacyId; Writer<<Reference;
		}
	TestTrue(TEXT("Legacy snapshot loads with retired records removed"),Saves->RestoreWorld(World,Save,Status));
	int32 RetiredCount=0,MineCount=0;
	for (TActorIterator<ABaseBuilding> It(World); It; ++It) if (!It->IsActorBeingDestroyed())
	{
		RetiredCount+=Manager->IsRetiredStandaloneBuilding(*It);
		MineCount+=It->IsA<AMiningMachine>();
	}
	TestEqual(TEXT("Obsolete standalone actor removed from world"),RetiredCount,0);
	TestEqual(TEXT("Real deposit mine remains"),MineCount,1);
	TestEqual(TEXT("Real mine still attached to original deposit"),Mine->GetResourceSource(),Deposit);
	TestEqual(TEXT("No construction-cost refund"),Resources->GetResourceAmount(EResourceType::Iron),17);
	TestEqual(TEXT("Stored silica not modified by actor removal"),Resources->GetResourceAmount(EResourceType::Silica),123);
	TestNull(TEXT("Worker no longer references removed actor"),Worker->GetAssignedBuilding());
	TestEqual(TEXT("Removed assignment slot cleared"),Worker->GetAssignedBuildingSlot(),INDEX_NONE);
	TestTrue(TEXT("Input save object remains unchanged"),Save->Actors.ContainsByPredicate([&](const auto& Record) { return Record.Id==LegacyId; }));
	Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
