#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionOutcomeTest,
 "SurviveThePlanet.Mission.Outcome",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionOutcomeTest::RunTest(const FString& Parameters)
{
	for (bool bVictory : {true, false})
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
		if (!TestNotNull(TEXT("Test world created"), World)) return false;
		UMissionConfidenceSubsystem* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>();
		if (!TestNotNull(TEXT("Mission subsystem created"), Mission)) return false;
		Mission->SetMissionProgress(-20);
		TestEqual(TEXT("Negative progress clamped"), Mission->GetMissionProgress(), 0);
		Mission->SetMissionProgress(9);
		TestTrue(TEXT("Incomplete bar remains playable"), Mission->GetMissionOutcome() == EMissionOutcome::InProgress);
		Mission->SetMissionConfidence(0.000001f);
		TestTrue(TEXT("Positive confidence remains playable"), Mission->GetMissionOutcome() == EMissionOutcome::InProgress);
		if (bVictory) Mission->AddMissionProgress(MAX_int32);
		else Mission->SetMissionConfidence(0.0f);
		const EMissionOutcome Expected = bVictory ? EMissionOutcome::Complete : EMissionOutcome::Failed;
		TestTrue(TEXT("Correct terminal outcome"), Mission->GetMissionOutcome() == Expected);
		TestEqual(TEXT("Progress clamped to target"), Mission->GetMissionProgress(), bVictory ? 10 : 9);
		Mission->SetMissionConfidence(bVictory ? 0.0f : 100.0f);
		Mission->SetMissionProgress(bVictory ? 0 : 10);
		Mission->Tick(1.0f);
		TestTrue(TEXT("Terminal outcome cannot change"), Mission->GetMissionOutcome() == Expected);
		World->DestroyWorld(false);
	}
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "Gameplay/UI/MissionResultWidget.h"
#include "SurviveThePlanetCameraPawn.h"
#include "SurviveThePlanetPlayerController.h"

// Start a gameplay PIE session. Events enter through the shared viewport,
// so this catches UI Only surviving travel instead of bypassing it via PlayerInput.
class FMissionRetryInputCommand : public IAutomationLatentCommand
{
public:
	explicit FMissionRetryInputCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
		}
		if (FPlatformTime::Seconds() - Started > 30.0)
		{
			Test->AddError(TEXT("Timed out waiting for Play Again to restore gameplay input"));
			return true;
		}
		if (!World) { Test->AddError(TEXT("Start a gameplay PIE session before running this test")); return true; }
		auto* PC = Cast<ASurviveThePlanetPlayerController>(World->GetFirstPlayerController());
		if (!PC || !World->GetGameViewport()) return false;
		auto* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>();
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (Stage == 0)
		{
			Mission->SetMissionConfidence(0.0f);
			Test->TestTrue(TEXT("Mission Failed blocks gameplay input"), Viewport->IgnoreInput());
			Test->TestTrue(TEXT("Mission Failed pauses the world"), UGameplayStatics::IsGamePaused(World));
			TArray<UUserWidget*> Results;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Results, UMissionResultWidget::StaticClass(), true);
			if (!Test->TestEqual(TEXT("One active result screen"), Results.Num(), 1)) return true;
			auto* RetryButton = Cast<UButton>(Results[0]->GetWidgetFromName(TEXT("RetryButton")));
			if (!Test->TestNotNull(TEXT("Play Again button"), RetryButton)) return true;
			PreviousController = PC;
			RetryButton->OnClicked.Broadcast();
			Stage = 1;
			return false;
		}
		if (Stage == 1)
		{
			if (PC == PreviousController.Get() || !PC->HasActorBegunPlay() || !PC->GetPawn()) return false;
			if (!Test->TestFalse(TEXT("Play Again restores viewport keyboard and mouse input"), Viewport->IgnoreInput())) return true;
			Test->TestFalse(TEXT("Restarted world is unpaused"), UGameplayStatics::IsGamePaused(World));
			Test->TestTrue(TEXT("Restarted mission is active"), Mission->GetMissionOutcome() == EMissionOutcome::InProgress);
			auto* Pawn = Cast<ASurviveThePlanetCameraPawn>(PC->GetPawn());
			if (!Test->TestNotNull(TEXT("Gameplay camera pawn"), Pawn)) return true;
			StartYaw = Pawn->GetCameraBoom()->GetComponentRotation().Yaw;
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.0f, -1, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, Viewport->Viewport));
			Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton, IE_Pressed, 1.0f, -1, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, Viewport->Viewport));
			InputStarted = FPlatformTime::Seconds();
			Stage = 2;
			return false;
		}
		if (FPlatformTime::Seconds() - InputStarted < 0.3) return false;
		auto* Pawn = Cast<ASurviveThePlanetCameraPawn>(PC->GetPawn());
		Test->TestTrue(TEXT("Keyboard reaches player after retry"), PC->IsInputKeyDown(EKeys::E));
		Test->TestTrue(TEXT("Mouse reaches player after retry"), PC->IsInputKeyDown(EKeys::RightMouseButton));
		Test->TestFalse(TEXT("Camera actually rotates after retry"), FMath::IsNearlyEqual(StartYaw, Pawn->GetCameraBoom()->GetComponentRotation().Yaw));
		Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.0f, -1, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, Viewport->Viewport));
		Viewport->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::RightMouseButton, IE_Released, 0.0f, -1, IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false, Viewport->Viewport));
		return true;
	}
private:
	FAutomationTestBase* Test;
	TWeakObjectPtr<ASurviveThePlanetPlayerController> PreviousController;
	double Started = FPlatformTime::Seconds();
	double InputStarted = 0.0;
	double StartYaw = 0.0;
	int32 Stage = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRetryInputTest, "SurviveThePlanet.Mission.RetryInputPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMissionRetryInputTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FMissionRetryInputCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Contracts/ContractOfferSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProgressionRewardsTest, "SurviveThePlanet.Mission.ProgressionRewards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProgressionRewardsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	auto* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>();
	auto* Objectives = World->GetSubsystem<UObjectiveSubsystem>();
	auto* Contracts = World->GetSubsystem<UContractOfferSubsystem>();
	auto* Resources = World->SpawnActor<AResourceManager>();
	Resources->SetCredits(0);
	Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
	TestEqual(TEXT("Initial objective grants configured progression"), Mission->GetMissionProgress(), 1);
	FSTPObjectiveRuntimeState ObjectiveState;
	Objectives->GetObjectiveState(TEXT("mission_build_communication"), ObjectiveState);
	TestTrue(TEXT("Objective reward guarded before notification"), ObjectiveState.bRewardsGranted);
	Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
	TestEqual(TEXT("Repeated build event does not grant twice"), Mission->GetMissionProgress(), 1);
	const TArray<FName> Choices = { TEXT("mission_deliver_water"), TEXT("mission_build_storage"), TEXT("mission_expand_mining") };
	TestTrue(TEXT("Select existing objective route"), Objectives->ConfirmMissionChoices(TEXT("mother_base_directives"), Choices));
	Objectives->SubmitResourceDelivered(EResourceType::Water, TEXT("HomeBase"), 199);
	TestEqual(TEXT("Partial objective gives no progression"), Mission->GetMissionProgress(), 1);
	Objectives->SubmitResourceDelivered(EResourceType::Water, TEXT("HomeBase"), 1);
	TestEqual(TEXT("Water objective grants +1"), Mission->GetMissionProgress(), 2);
	Objectives->SubmitBuildingBuilt(ESTPBuildingType::EnergyStorage, 2);
	TestEqual(TEXT("Storage objective grants +1"), Mission->GetMissionProgress(), 3);
	Objectives->SubmitBuildingBuilt(ESTPBuildingType::MiningMachine, 2);
	TestEqual(TEXT("Mining objective grants a different amount (+2)"), Mission->GetMissionProgress(), 5);
	const auto Offers = Contracts->GetContractOffers();
	if (!TestEqual(TEXT("Existing agreement loads"), Offers.Num(), 1)) { World->DestroyWorld(false); return false; }
	const auto& Offer = Offers[0];
	for (const auto& Tier : Offer.Tiers)
	{
		TestTrue(TEXT("Select agreement tier"), Contracts->ToggleContractSelection(Offer.Id, Tier.Id));
		const int32 Before = Mission->GetMissionProgress();
		TestTrue(TEXT("Accept agreement"), Contracts->ConfirmSelectedContract());
		TestEqual(TEXT("Accepting gives no reward"), Mission->GetMissionProgress(), Before);
		Resources->SetResourceAmount(EResourceType::ControlChip, 0);
		const int32 CreditsBefore = Resources->GetCredits();
		TestFalse(TEXT("Insufficient resources cannot deliver"), Contracts->DeliverSelectedContract());
		TestEqual(TEXT("Invalid delivery does not grant credits"), Resources->GetCredits(), CreditsBefore);
		Resources->SetResourceAmount(EResourceType::ControlChip, Tier.DeliveryAmount * Tier.DeliveryCount);
		for (int32 Delivery = 0; Delivery < Tier.DeliveryCount; ++Delivery)
		{
			TestTrue(TEXT("Real resource shipment delivered"), Contracts->DeliverSelectedContract());
			if (Delivery + 1 < Tier.DeliveryCount) TestEqual(TEXT("Partial agreement gives no completion progression"), Mission->GetMissionProgress(), Before);
		}
		TestTrue(TEXT("All shipments complete agreement"), Contracts->GetSelectedContractState().State == ESTPContractState::Completed);
		TestEqual(TEXT("All shipment resources consumed"), Resources->GetResourceAmount(EResourceType::ControlChip), 0);
		TestEqual(TEXT("Configured tier reward applied"), Mission->GetMissionProgress(), FMath::Min(10, Before + Tier.GetMissionProgressReward()));
		const int32 CreditsAfter = Resources->GetCredits();
		int32 ExpectedCredits = CreditsBefore;
		for (const auto& Effect : Tier.DeliveryRewards) if (Effect.Type == ESTPContractEffectType::Credits) ExpectedCredits += Effect.Amount * Tier.DeliveryCount;
		for (const auto& Effect : Tier.CompletionBonus) if (Effect.Type == ESTPContractEffectType::Credits) ExpectedCredits += Effect.Amount;
		TestEqual(TEXT("All monetary rewards settle before progression can end mission"), CreditsAfter, ExpectedCredits);
		TestFalse(TEXT("Completed agreement cannot deliver again"), Contracts->DeliverSelectedContract());
		TestEqual(TEXT("Duplicate completion does not grant credits"), Resources->GetCredits(), CreditsAfter);
	}
	TestTrue(TEXT("Combined objective and agreement rewards can win"), Mission->GetMissionOutcome() == EMissionOutcome::Complete);
	TestEqual(TEXT("Progression clamps to full bar"), Mission->GetMissionProgress(), 10);
	World->DestroyWorld(false);

	World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
	Mission = World->GetSubsystem<UMissionConfidenceSubsystem>();
	Contracts = World->GetSubsystem<UContractOfferSubsystem>();
	Resources = World->SpawnActor<AResourceManager>();
	Resources->SetCredits(0);
	Resources->SetResourceAmount(EResourceType::ControlChip, 0);
	Contracts->ToggleContractSelection(Offer.Id, TEXT("standard"));
	Contracts->ConfirmSelectedContract();
	const float Interval = Contracts->GetSelectedContractState().SecondsUntilDelivery;
	UGameplayStatics::SetGlobalTimeDilation(World, 0.0001f);
	Contracts->Tick(Interval);
	TestEqual(TEXT("Simulation pause freezes delivery timer"), Contracts->GetSelectedContractState().SecondsUntilDelivery, Interval);
	UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	Contracts->Tick(Interval);
	TestEqual(TEXT("Missed deadline recorded"), Contracts->GetSelectedContractState().DeliveriesMissed, 1);
	TestEqual(TEXT("Missed deadline grants no progression"), Mission->GetMissionProgress(), 0);
	TestEqual(TEXT("Missed deadline applies configured confidence penalty"), Mission->GetMissionConfidence(), 98.0f);
	Resources->SetResourceAmount(EResourceType::ControlChip, 24);
	for (int32 I = 0; I < 3; ++I) TestTrue(TEXT("Remaining shipment can still be delivered"), Contracts->DeliverSelectedContract());
	TestTrue(TEXT("Agreement with a missed shipment fails"), Contracts->GetSelectedContractState().State == ESTPContractState::Failed);
	TestEqual(TEXT("Failed agreement receives no completion progression"), Mission->GetMissionProgress(), 0);
	TestEqual(TEXT("Failed agreement gets delivery credits only"), Resources->GetCredits(), 450);
	Mission->SetMissionConfidence(0.0f);
	TestFalse(TEXT("Cannot accept agreements after mission failure"), Contracts->ConfirmSelectedContract());
	World->DestroyWorld(false);
	return true;
}
#endif
