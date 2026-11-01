#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerInput.h"
#include "Widgets/SOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Gameplay/UI/MissionChoiceWidget.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "SurviveThePlanetPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionPopupTest,"SurviveThePlanet.UI.MissionPopup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionPopupTest::RunTest(const FString& Parameters)
{
 auto* GI=NewObject<UGameInstance>(GEngine); GI->InitializeStandalone();
 auto* World=GI->GetWorld(); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL());
 auto* Viewport=NewObject<UGameViewportClient>(GEngine); GI->GetWorldContext()->GameViewport=Viewport;
 Viewport->Init(*GI->GetWorldContext(),GI,false); Viewport->SetViewportOverlayWidget(nullptr,SNew(SOverlay));
 FString Error; auto* LocalPlayer=GI->CreateLocalPlayer(0,Error,false);
 auto* PC=World->SpawnActor<ASurviveThePlanetPlayerController>(); PC->SetPlayer(LocalPlayer); PC->PlayerInput=NewObject<UPlayerInput>(PC);
 auto* HUD=CreateWidget<UResourceDisplayWidget>(PC); HUD->AddToViewport();
 auto* Resources=World->SpawnActor<AResourceManager>();
 auto* Objectives=World->GetSubsystem<UObjectiveSubsystem>();
 Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
 Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
 World->SpawnActor<ACargoBay>()->SetConstructionProgress(1);
 UClass* Class=LoadClass<UMissionChoiceWidget>(nullptr,TEXT("/Game/UI/Mission/WBP_MissionChoicePopup.WBP_MissionChoicePopup_C"));
 auto* Popup=CreateWidget<UMissionChoiceWidget>(PC,Class); Popup->AddToViewport(20);
 HUD->SetGameTimeScale(4);
 Popup->OpenPopup();
 TestTrue(TEXT("Popup pauses world"),UGameplayStatics::IsGamePaused(World));
 TestTrue(TEXT("Popup pauses real-time simulation clock"),HUD->IsSimulationPaused());
 TestTrue(TEXT("Popup blocks camera input"),PC->IsCameraInputBlocked());
 const double Minutes=HUD->TotalGameMinutes; HUD->NativeTick(FGeometry(),2);
 TestEqual(TEXT("Clock stays frozen despite UI ticks"),HUD->TotalGameMinutes,Minutes);
 TArray<UWidget*> Widgets; Popup->WidgetTree->GetAllWidgets(Widgets);
 TArray<UMissionChoiceButton*> Deliveries;
 for(auto* Widget:Widgets) if(auto* Button=Cast<UMissionChoiceButton>(Widget);Button&&Button->GetSlotIndex()==INDEX_NONE) Deliveries.Add(Button);
 TestEqual(TEXT("Each active delivery has a button"),Deliveries.Num(),3);
 TestNotNull(TEXT("Full-screen input blocker exists"),Popup->WidgetTree->FindWidget(TEXT("MissionModalBlocker")));
 if(!Deliveries.IsEmpty())
 {
  auto* Button=Deliveries[0]; FSTPObjectiveDefinition Definition; Objectives->GetObjectiveDefinition(Button->GetObjectiveId(),Definition);
  const auto Cost=Definition.Conditions[0]; Resources->SetResourceAmount(Cost.ResourceType,0);
  Popup->NativeTick(FGeometry(),0); TestFalse(TEXT("Missing goods disables shipment"),Button->GetIsEnabled());
  Resources->SetResourceAmount(Cost.ResourceType,Cost.RequiredAmount);
  Popup->NativeTick(FGeometry(),0); TestTrue(TEXT("Enough goods enables shipment while paused"),Button->GetIsEnabled());
  Button->OnClicked.Broadcast();
  TestEqual(TEXT("UI delivery consumes required goods"),Resources->GetResourceAmount(Cost.ResourceType),0);
  TestEqual(TEXT("UI delivery completes one request"),Objectives->GetActiveObjectives().Num(),2);
  TestTrue(TEXT("Delivery keeps popup paused"),UGameplayStatics::IsGamePaused(World));
 }
 Popup->ClosePopupIfOpen();
 TestFalse(TEXT("Closing resumes world"),UGameplayStatics::IsGamePaused(World));
 TestFalse(TEXT("Closing resumes simulation clock"),HUD->IsSimulationPaused());
 TestEqual(TEXT("Previous speed restored"),UGameplayStatics::GetGlobalTimeDilation(World),4.0f);
 HUD->HandlePauseTimeClicked(); const float PausedRate=UGameplayStatics::GetGlobalTimeDilation(World);
 Popup->OpenPopup(); Popup->ClosePopupIfOpen();
 TestTrue(TEXT("Manual pause remains selected"),HUD->IsSimulationPaused());
 TestEqual(TEXT("Manual pause rate restored"),UGameplayStatics::GetGlobalTimeDilation(World),PausedRate);
 Popup->RemoveFromParent(); HUD->RemoveFromParent(); GI->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif
