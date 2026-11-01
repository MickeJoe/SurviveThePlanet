#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Resources/ResourceManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionPacketsTest,"SurviveThePlanet.MissionPackets.ScheduleAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionPacketsTest::RunTest(const FString& Parameters)
{
 auto* GI=NewObject<UGameInstance>(GEngine); GI->InitializeStandalone(MakeUniqueObjectName(nullptr,UWorld::StaticClass()),GetTransientPackage());
 auto* World=GI->GetWorld(); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->GetWorldSettings()->NotifyBeginPlay();
 World->SpawnActor<AResourceManager>();
 auto* Objectives=World->GetSubsystem<UObjectiveSubsystem>();
 const auto Packets=Objectives->GetMissionChoiceGroups();
 if(!TestEqual(TEXT("Ten configured packets visible including locked ones"),Packets.Num(),10)) return false;
 auto Candidate=[&](int32 Index,int32 Option=0){return Packets[Index].Slots[0].ObjectiveIds[Option];};
 TestFalse(TEXT("Cargo prerequisite cannot be bypassed"),Objectives->ConfirmMissionChoiceSlot(Packets[0].Id,0,Candidate(0)));
 Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule); Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
 Objectives->UpdateMissionClock(479);
 TestFalse(TEXT("Early choice rejected"),Objectives->ConfirmMissionChoiceSlot(Packets[0].Id,0,Candidate(0)));
 Objectives->UpdateMissionClock(480);
 TestTrue(TEXT("First packet unlocked on exact boundary"),Objectives->IsMissionChoiceGroupUnlocked(Packets[0].Id));
 TestTrue(TEXT("Second packet unlocks at same time"),Objectives->IsMissionChoiceGroupUnlocked(Packets[1].Id));
 TestFalse(TEXT("Future packet remains locked"),Objectives->IsMissionChoiceGroupUnlocked(Packets[2].Id));
 TestTrue(TEXT("Choose first option"),Objectives->ConfirmMissionChoiceSlot(Packets[0].Id,0,Candidate(0)));
 TestFalse(TEXT("Alternative cannot be chosen after acceptance"),Objectives->ConfirmMissionChoiceSlot(Packets[0].Id,0,Candidate(0,1)));
 TestFalse(TEXT("Alternative cannot activate through public API"),Objectives->ActivateObjective(Candidate(0,1)));
 Objectives->UpdateMissionClock(1920);
 TestTrue(TEXT("New packet unlocks while previous mission remains active"),Objectives->IsMissionChoiceGroupUnlocked(Packets[2].Id));
 Objectives->UpdateMissionClock(10000);
 for(int32 Index=1;Index<10;++Index)
  TestTrue(TEXT("All ten choices can coexist"),Objectives->ConfirmMissionChoiceSlot(Packets[Index].Id,0,Candidate(Index,Index%2)));
 TestEqual(TEXT("Ten accepted missions remain active"),Objectives->GetActiveObjectives().Num(),10);
 // Simulate an older slot that still contains automatically assigned requests.
 Objectives->RuntimeStates.FindChecked(TEXT("mother_base_polymer")).State=ESTPObjectiveState::Active;
 Objectives->RuntimeStates.FindChecked(TEXT("mother_base_water")).State=ESTPObjectiveState::Active;
 auto* Saves=GI->GetSubsystem<UColonySaveSubsystem>(); FText Status;
 auto* Snapshot=Saves->CaptureWorld(World,Status); TArray<uint8> Bytes;
 TestTrue(TEXT("Packet state serializes to save bytes"),UGameplayStatics::SaveGameToMemory(Snapshot,Bytes));
 auto* Loaded=Cast<UColonySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
 Objectives->ReloadObjectives();
 TestTrue(TEXT("Save restores packet choices"),Saves->RestoreWorld(World,Loaded,Status));
 FSTPObjectiveRuntimeState RetiredState;
 TestFalse(TEXT("Legacy Polymer request removed during load"),Objectives->GetObjectiveState(TEXT("mother_base_polymer"),RetiredState));
 TestFalse(TEXT("Legacy Water request removed during load"),Objectives->GetObjectiveState(TEXT("mother_base_water"),RetiredState));
 TestEqual(TEXT("Clock restores"),Objectives->GetMissionGameMinutes(),10000.0);
 TestEqual(TEXT("All ten active missions restored"),Objectives->GetActiveObjectives().Num(),10);
 for(int32 Index=0;Index<10;++Index)
 {
  FSTPMissionChoiceGroupState State; Objectives->GetMissionChoiceGroupState(Packets[Index].Id,State);
  TestTrue(TEXT("Accepted packet remains permanently chosen"),State.bLocked);
  TestEqual(TEXT("Exact choice survives save"),State.SelectedObjectiveIds[0],Candidate(Index,Index%2));
 }
 GI->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif
