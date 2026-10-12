#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "SurviveThePlanetPlayerController.h"
#include "SurviveThePlanetCameraPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMenuInputTest,
 "SurviveThePlanet.Camera.CheatMenuInput",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraMenuInputTest::RunTest(const FString& Parameters)
{
 UGameInstance* GI = NewObject<UGameInstance>(GEngine);
 GI->InitializeStandalone();
 UWorld* World = GI->GetWorld();
 World->InitializeActorsForPlay(FURL());
 UGameViewportClient* Viewport = NewObject<UGameViewportClient>(GEngine);
 GI->GetWorldContext()->GameViewport = Viewport;
 Viewport->Init(*GI->GetWorldContext(), GI, false);
 Viewport->SetViewportOverlayWidget(nullptr, SNew(SOverlay));
 FString Error;
 ULocalPlayer* LocalPlayer = GI->CreateLocalPlayer(0, Error, false);
 ASurviveThePlanetPlayerController* PC = World->SpawnActor<ASurviveThePlanetPlayerController>();
 PC->SetPlayer(LocalPlayer);
 PC->PlayerInput = NewObject<UPlayerInput>(PC);
 ASurviveThePlanetCameraPawn* Pawn = World->SpawnActor<ASurviveThePlanetCameraPawn>();
 PC->Possess(Pawn);
 auto Key = [PC](FKey Key, EInputEvent Event)
 {
  PC->PlayerInput->InputKey(FInputKeyParams(Key, Event, 1.0));
  TArray<UInputComponent*> Stack;
  PC->PlayerInput->ProcessInputStack(Stack, 0.1f, false);
 };
 for (int32 Attempt = 0; Attempt < 2; ++Attempt)
 {
  const double StartYaw = Pawn->GetCameraBoom()->GetComponentRotation().Yaw;
  Key(EKeys::E, IE_Pressed);
  Pawn->Tick(0.1f);
  TestFalse(TEXT("E rotates the camera during gameplay"), FMath::IsNearlyEqual(StartYaw, Pawn->GetCameraBoom()->GetComponentRotation().Yaw));
  PC->ToggleCheatMenu();
  TestTrue(TEXT("Cheat menu blocks navigation"), PC->IsCameraInputBlocked());
  TestFalse(TEXT("Opening menu clears held E"), PC->IsInputKeyDown(EKeys::E));
  const double MenuYaw = Pawn->GetCameraBoom()->GetComponentRotation().Yaw;
  const FVector MenuPosition = Pawn->GetActorLocation();
  // Reproduce a gameplay press whose release is consumed by a text field.
  Key(EKeys::E, IE_Pressed);
  Key(EKeys::W, IE_Pressed);
  Pawn->Tick(0.5f);
  TestEqual(TEXT("Typing cannot rotate the camera"), Pawn->GetCameraBoom()->GetComponentRotation().Yaw, MenuYaw);
  TestEqual(TEXT("Typing cannot pan the camera"), Pawn->GetActorLocation(), MenuPosition);
  PC->ToggleCheatMenu();
  TestFalse(TEXT("Closing menu restores navigation"), PC->IsCameraInputBlocked());
  TestFalse(TEXT("Closing clears swallowed-release key state"), PC->IsInputKeyDown(EKeys::E));
  Pawn->Tick(0.5f);
  TestEqual(TEXT("No residual rotation after closing"), Pawn->GetCameraBoom()->GetComponentRotation().Yaw, MenuYaw);
  Key(EKeys::E, IE_Released);
  Pawn->Tick(0.5f);
  TestEqual(TEXT("Late release cannot create reverse rotation"), Pawn->GetCameraBoom()->GetComponentRotation().Yaw, MenuYaw);
 }
 GI->Shutdown();
 GEngine->DestroyWorldContext(World);
 World->DestroyWorld(false);
 return true;
}
#endif
