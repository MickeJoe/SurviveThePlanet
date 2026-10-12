#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerInput.h"
#include "Widgets/SOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/UI/PauseMenuWidget.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "Gameplay/UI/BuildingInfoWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseMenuTest,
 "SurviveThePlanet.UI.PauseMenu",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPauseMenuTest::RunTest(const FString& Parameters)
{
	TArray<uint8> OriginalSave;
	const FString SaveSlot = TEXT("SurviveThePlanet_GameTime");
	const bool bHadSave = UGameplayStatics::LoadDataFromSlot(OriginalSave, SaveSlot, 0);
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	World->SetGameMode(FURL());
	World->InitializeActorsForPlay(FURL());
	UGameViewportClient* Viewport = NewObject<UGameViewportClient>(GEngine);
	GI->GetWorldContext()->GameViewport = Viewport;
	Viewport->Init(*GI->GetWorldContext(), GI, false);
	Viewport->SetViewportOverlayWidget(nullptr, SNew(SOverlay));
	FString Error;
	ULocalPlayer* LocalPlayer = GI->CreateLocalPlayer(0, Error, false);
	auto* PC = World->SpawnActor<ASurviveThePlanetPlayerController>();
	PC->SetPlayer(LocalPlayer);
	PC->PlayerInput = NewObject<UPlayerInput>(PC);
	auto* HUD = CreateWidget<UResourceDisplayWidget>(PC);
	HUD->AddToViewport();
	PC->SetSelectedActor(nullptr);
	PC->SetActiveBuildTool(ESTPBuildTool::None);
	for (float Speed : {1.0f, 1.5f, 2.0f, 3.0f})
	{
		HUD->SetGameTimeScale(Speed);
		PC->OnCancelBuildToolPressed();
		TestTrue(TEXT("Escape opens menu"), PC->PauseMenuWidget && PC->PauseMenuWidget->IsInViewport());
		TestTrue(TEXT("World is paused"), UGameplayStatics::IsGamePaused(World));
		TestTrue(TEXT("HUD clock is paused"), HUD->IsSimulationPaused());
		TestTrue(TEXT("Menu blocks camera input"), PC->IsCameraInputBlocked());
		const double Minutes = HUD->TotalGameMinutes;
		HUD->NativeTick(FGeometry(), 2.0f);
		TestEqual(TEXT("Real-time HUD tick cannot advance clock"), HUD->TotalGameMinutes, Minutes);
		PC->OpenPauseMenu();
		PC->ResumeGame();
		TestFalse(TEXT("Resume unpauses world"), UGameplayStatics::IsGamePaused(World));
		TestFalse(TEXT("Resume unpauses HUD clock"), HUD->IsSimulationPaused());
		TestEqual(TEXT("Resume restores exact speed"), UGameplayStatics::GetGlobalTimeDilation(World), Speed);
		TestFalse(TEXT("Resume restores camera input"), PC->IsCameraInputBlocked());
		PC->OnCancelBuildToolPressed();
		PC->OnCancelBuildToolPressed();
		TestEqual(TEXT("Escape also restores exact speed"), UGameplayStatics::GetGlobalTimeDilation(World), Speed);
	}
	HUD->SetGameTimeScale(1.5f);
	HUD->HandlePauseTimeClicked();
	const float PausedRate = UGameplayStatics::GetGlobalTimeDilation(World);
	PC->OnCancelBuildToolPressed();
	PC->ResumeGame();
	TestTrue(TEXT("Player-selected pause survives Resume"), HUD->IsSimulationPaused());
	TestEqual(TEXT("Paused simulation dilation is preserved"), UGameplayStatics::GetGlobalTimeDilation(World), PausedRate);
	TestEqual(TEXT("Selected speed is still 1.5"), HUD->TimeScale, 1.5f);
	HUD->HandlePauseTimeClicked();
	TestEqual(TEXT("Unpausing returns to selected speed"), UGameplayStatics::GetGlobalTimeDilation(World), 1.5f);
	PC->ResumeGame();
	TestEqual(TEXT("Redundant Resume leaves speed intact"), UGameplayStatics::GetGlobalTimeDilation(World), 1.5f);
	HUD->RemoveFromParent();
	GI->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	if (bHadSave) UGameplayStatics::SaveDataToSlot(OriginalSave, SaveSlot, 0);
	else UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
	return true;
}
#endif
