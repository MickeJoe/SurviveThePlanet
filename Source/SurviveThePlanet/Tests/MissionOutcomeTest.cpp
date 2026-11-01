#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "Gameplay/Buildings/CargoBay.h"

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
    TestTrue(TEXT("Supply requests hidden before Cargo Bay"), Objectives->GetMissionChoiceGroups().IsEmpty());
    TestFalse(TEXT("Cannot bypass progression gate"), Objectives->ConfirmMissionChoiceSlot(TEXT("mother_base_directives"), 0, TEXT("mother_base_iron")));
    Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
    TestEqual(TEXT("Uplink grants progression"), Mission->GetMissionProgress(), 1);
    Objectives->SubmitBuildingBuilt(ESTPBuildingType::CommunicationModule);
    TestEqual(TEXT("Uplink rewards cannot repeat"), Mission->GetMissionProgress(), 1);
    Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
    TestEqual(TEXT("Cargo grants progression"), Mission->GetMissionProgress(), 2);
    const FName Package = Objectives->GetSelectedCargoPackage();
    TestTrue(TEXT("Cargo assigns A or B"), Package == TEXT("A") || Package == TEXT("B"));
    const auto Active = Objectives->GetActiveObjectives();
    if (!TestEqual(TEXT("Three parallel requests"), Active.Num(), 3)) { World->DestroyWorld(false); return false; }
    FSTPObjectiveDefinition Request;
    Objectives->GetObjectiveDefinition(Active[0].ObjectiveId, Request);
    const auto Condition = Request.Conditions[0];
    TestTrue(TEXT("Mother Base demands inventory delivery"), Condition.Type == ESTPObjectiveConditionType::DeliverResource);
    TestEqual(TEXT("Demand destination"), Condition.Destination, FName(TEXT("HomeBase")));
    Objectives->SubmitResourceDelivered(Condition.ResourceType, Condition.Destination, Condition.RequiredAmount - 1);
    TestEqual(TEXT("Partial delivery gives no progression"), Mission->GetMissionProgress(), 2);
    const int32 ChipsBefore = Resources->GetResourceAmount(EResourceType::ControlChip);
    TestFalse(TEXT("Delivery requires physical Cargo Bay"), Objectives->CanDeliverObjective(Request.Id));
    auto* DeliveryBay = World->SpawnActor<ACargoBay>();
    DeliveryBay->SetConstructionProgress(1);
    Resources->SetResourceAmount(Condition.ResourceType, 0);
    TestFalse(TEXT("Delivery rejects insufficient inventory"), Objectives->DeliverObjective(Request.Id));
    Resources->SetResourceAmount(Condition.ResourceType, 1);
    TestTrue(TEXT("Remaining one-unit shipment is affordable"), Objectives->CanDeliverObjective(Request.Id));
    TestTrue(TEXT("Real Mother Base shipment succeeds"), Objectives->DeliverObjective(Request.Id));
    TestEqual(TEXT("Mother Base shipment consumes inventory"), Resources->GetResourceAmount(Condition.ResourceType), 0);
    TestFalse(TEXT("Completed objective cannot spend or reward again"), Objectives->DeliverObjective(Request.Id));
    DeliveryBay->Destroy();
    TestEqual(TEXT("Completed request grants +2"), Mission->GetMissionProgress(), 4);
    TestTrue(TEXT("Imported boards replenished"), Resources->GetResourceAmount(EResourceType::ControlChip) > ChipsBefore);
    Objectives->SubmitResourceDelivered(Condition.ResourceType, Condition.Destination, Condition.RequiredAmount);
    TestEqual(TEXT("Duplicate delivery event cannot reward twice"), Mission->GetMissionProgress(), 4);
    Objectives->SubmitBuildingBuilt(ESTPBuildingType::CargoBay);
    TestEqual(TEXT("Repeated cargo never rerolls package"), Objectives->GetSelectedCargoPackage(), Package);
    Objectives->Tick(7200);
    TestEqual(TEXT("Unfinished requests retained without expiry or replacement"), Objectives->GetActiveObjectives().Num(), 2);
    TestFalse(TEXT("Contracts gated without actual completed Cargo Bay"), Contracts->AreContractsAvailable());
    auto* CargoBay = World->SpawnActor<ACargoBay>();
    CargoBay->SetConstructionProgress(0.5f);
    TestFalse(TEXT("Unfinished Cargo Bay keeps contracts locked"), Contracts->AreContractsAvailable());
    CargoBay->SetConstructionProgress(1.0f);
    const auto Offers = Contracts->GetContractOffers();
    TestEqual(TEXT("Five contract families"), Offers.Num(), 5);
    int32 TierCount = 0;
    for (const auto& Offer : Offers)
    {
        TestEqual(TEXT("Three tiers per family"), Offer.Tiers.Num(), 3);
        TierCount += Offer.Tiers.Num();
        TestTrue(TEXT("Boards are rewards, never early contract inputs"), Offer.ItemId != TEXT("ControlChip"));
    }
    TestEqual(TEXT("Fifteen contract commitments"), TierCount, 15);
    const auto Available = Contracts->GetContractOffersForTrader(TEXT("helix_industrial"));
    if (!TestEqual(TEXT("One stable offer per faction"), Available.Num(), 1)) { World->DestroyWorld(false); return false; }
    const auto Offer = Available[0];
    TestTrue(TEXT("Reload keeps current run's offer"), Contracts->ReloadContractOffers());
    TestEqual(TEXT("Panel reopen/reload cannot reroll"), Contracts->GetContractOffersForTrader(Offer.TraderId)[0].Id, Offer.Id);
    for (const auto& Candidate : Offers)
        if (Candidate.TraderId == Offer.TraderId && Candidate.Id != Offer.Id)
            TestFalse(TEXT("Hidden variant cannot be accepted directly"), Contracts->ToggleContractSelection(Candidate.Id, TEXT("standard")));
    const auto Resource = static_cast<EResourceType>(StaticEnum<EResourceType>()->GetValueByNameString(Offer.ItemId.ToString()));
    for (const auto& Tier : Offer.Tiers)
    {
        TestTrue(TEXT("Select visible tier"), Contracts->ToggleContractSelection(Offer.Id, Tier.Id));
        const int32 Before = Mission->GetMissionProgress();
        TestTrue(TEXT("Accept agreement"), Contracts->ConfirmSelectedContract());
        TestEqual(TEXT("Acceptance grants no rewards"), Mission->GetMissionProgress(), Before);
        Resources->SetResourceAmount(Resource, 0);
        TestFalse(TEXT("Cannot deliver nonexistent inventory"), Contracts->DeliverSelectedContract());
        Resources->SetResourceAmount(Resource, Tier.DeliveryAmount * Tier.DeliveryCount);
        const int32 CreditsBefore = Resources->GetCredits();
        for (int32 I = 0; I < Tier.DeliveryCount; ++I)
        {
            TestTrue(TEXT("Real inventory consumed"), Contracts->DeliverSelectedContract());
            if (I + 1 < Tier.DeliveryCount) TestEqual(TEXT("No partial progression reward"), Mission->GetMissionProgress(), Before);
        }
        TestTrue(TEXT("Agreement completes"), Contracts->GetSelectedContractState().State == ESTPContractState::Completed);
        TestEqual(TEXT("All shipment goods consumed"), Resources->GetResourceAmount(Resource), 0);
        TestEqual(TEXT("Configured completion progression"), Mission->GetMissionProgress(), FMath::Min(10, Before + Tier.GetMissionProgressReward()));
        int32 ExpectedCredits = CreditsBefore;
        for (const auto& E : Tier.DeliveryRewards) if (E.Type == ESTPContractEffectType::Credits) ExpectedCredits += E.Amount * Tier.DeliveryCount;
        for (const auto& E : Tier.CompletionBonus) if (E.Type == ESTPContractEffectType::Credits) ExpectedCredits += E.Amount;
        TestEqual(TEXT("Credits settle before victory"), Resources->GetCredits(), ExpectedCredits);
        TestFalse(TEXT("Cannot duplicate completion rewards"), Contracts->DeliverSelectedContract());
    }
    TestTrue(TEXT("Progression remains winnable"), Mission->GetMissionOutcome() == EMissionOutcome::Complete);
    World->DestroyWorld(false);

    // A separate run verifies deadlines and recovery without prior rewards.
    World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(nullptr, UWorld::StaticClass()), GetTransientPackage());
    Mission = World->GetSubsystem<UMissionConfidenceSubsystem>();
    Contracts = World->GetSubsystem<UContractOfferSubsystem>();
    Resources = World->SpawnActor<AResourceManager>();
    Resources->SetCredits(0);
    World->SpawnActor<ACargoBay>()->SetConstructionProgress(1);
    const auto FailureOffer = Contracts->GetContractOffersForTrader(TEXT("helix_industrial"))[0];
    const auto Tier = FailureOffer.Tiers[0];
    const auto FailureResource = static_cast<EResourceType>(StaticEnum<EResourceType>()->GetValueByNameString(FailureOffer.ItemId.ToString()));
    Resources->SetResourceAmount(FailureResource, 0);
    TestTrue(TEXT("Select failure scenario"), Contracts->ToggleContractSelection(FailureOffer.Id, Tier.Id));
    TestTrue(TEXT("Accept failure scenario"), Contracts->ConfirmSelectedContract());
    const float Interval = Contracts->GetSelectedContractState().SecondsUntilDelivery;
    UGameplayStatics::SetGlobalTimeDilation(World, 0.0001f);
    Contracts->Tick(Interval);
    TestEqual(TEXT("Pause freezes deadline"), Contracts->GetSelectedContractState().SecondsUntilDelivery, Interval);
    UGameplayStatics::SetGlobalTimeDilation(World, 1);
    Contracts->Tick(Interval);
    TestEqual(TEXT("Miss recorded"), Contracts->GetSelectedContractState().DeliveriesMissed, 1);
    TestEqual(TEXT("Configured standard miss penalty"), Mission->GetMissionConfidence(), 97.0f);
    Resources->SetResourceAmount(FailureResource, Tier.DeliveryAmount * (Tier.DeliveryCount - 1));
    for (int32 I = 1; I < Tier.DeliveryCount; ++I) TestTrue(TEXT("Can recover remaining deliveries"), Contracts->DeliverSelectedContract());
    TestTrue(TEXT("Missed agreement receives no completion bonus"), Contracts->GetSelectedContractState().State == ESTPContractState::Failed);
    TestEqual(TEXT("Failed agreement grants no progression"), Mission->GetMissionProgress(), 0);
    Mission->SetMissionConfidence(0);
    TestFalse(TEXT("Cannot accept after mission failure"), Contracts->ConfirmSelectedContract());
    World->DestroyWorld(false);
    return true;
}
#endif
