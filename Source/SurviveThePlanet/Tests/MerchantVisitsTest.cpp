#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Gameplay/Planet/PlanetDefinition.h"
#include "Gameplay/Trading/TradeSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPMerchantVisitsTest,
	"SurviveThePlanet.Trading.MerchantVisits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPMerchantVisitsTest::RunTest(const FString& Parameters)
{
	UWorld* World = NewObject<UWorld>();
	UTradeSubsystem* Trading = NewObject<UTradeSubsystem>(World);
	UPlanetDefinition* Planet = LoadObject<UPlanetDefinition>(nullptr,
		TEXT("/Game/Data/Planet/DA_PlanetDefinition.DA_PlanetDefinition"));
	if (!TestNotNull(TEXT("Planet template"), Planet)) return false;
	Trading->Planet = Planet;
	Trading->bCargoBayReady = true;
	const auto& Visits = Planet->MerchantVisits.Visits;
	if (!TestEqual(TEXT("Four authored visiting merchants"), Visits.Num(), 4)) return false;
	TestEqual(TEXT("Eight-day repeating sequence"), Planet->MerchantVisits.RepeatCycleDays, 8.0f);
	TSet<FName> MerchantIds;
	for (int32 Index = 0; Index < Visits.Num(); ++Index)
	{
		const auto& Visit = Visits[Index];
		if (!TestNotNull(TEXT("Merchant definition"), Visit.Merchant.Get())) return false;
		MerchantIds.Add(Visit.Merchant->Id);
		TestNotNull(TEXT("Merchant portrait"), Visit.Merchant->Portrait.Get());
		TestTrue(TEXT("Merchant has an authored assortment"), !Visit.Merchant->OfferedItemIds.IsEmpty());
		TestEqual(TEXT("Arrival every other day after first day"), Visit.ArrivalAfterHours, 24.0f + Index * 48.0f);
		TestEqual(TEXT("Eight-hour stay"), Visit.StayHours, 8.0f);
		const double Arrival = Visit.ArrivalAfterHours * 60.0;
		Trading->MinutesSinceCargoBay = Arrival - 1.0;
		Trading->RefreshVisitState();
		TestFalse(TEXT("Merchant absent one minute before arrival"), Trading->VisitState.bPresent);
		TestEqual(TEXT("Correct next merchant"), Trading->VisitState.Merchant.Get(), Visit.Merchant.Get());
		TestEqual(TEXT("One game minute remaining"), Trading->VisitState.RemainingGameMinutes, 1.0);
		Trading->MinutesSinceCargoBay = Arrival;
		Trading->RefreshVisitState();
		TestTrue(TEXT("Merchant present exactly at arrival"), Trading->VisitState.bPresent);
		TestEqual(TEXT("Correct arriving merchant"), Trading->VisitState.Merchant.Get(), Visit.Merchant.Get());
		TestEqual(TEXT("Departure countdown"), Trading->VisitState.RemainingGameMinutes, 480.0);
		Trading->MinutesSinceCargoBay = Arrival + 480.0;
		Trading->RefreshVisitState();
		TestFalse(TEXT("Merchant departs exactly after eight hours"), Trading->VisitState.bPresent);
	}
	TestEqual(TEXT("Distinct merchant IDs"), MerchantIds.Num(), 4);
	Trading->MinutesSinceCargoBay = 1440.0;
	Trading->RefreshVisitState();
	Trading->TraderStock.FindOrAdd(Visits[0].Merchant->Id).Add(TEXT("test_item"), 42);
	Trading->MinutesSinceCargoBay += 1.0;
	Trading->RefreshVisitState();
	TestEqual(TEXT("Stock remains throughout one visit"),
		Trading->TraderStock.FindChecked(Visits[0].Merchant->Id).FindRef(TEXT("test_item")), 42);
	Trading->MinutesSinceCargoBay = 1440.0 + 8.0 * 1440.0;
	Trading->RefreshVisitState();
	TestTrue(TEXT("First merchant returns after eight days"), Trading->VisitState.bPresent);
	TestEqual(TEXT("Sequence repeats with the first merchant"), Trading->VisitState.Merchant.Get(), Visits[0].Merchant.Get());
	TestFalse(TEXT("Returning merchant receives fresh stock"), Trading->TraderStock.Contains(Visits[0].Merchant->Id));
	Trading->bCargoBayReady = false;
	Trading->RefreshVisitState();
	TestFalse(TEXT("Cargo Bay unavailable disables visits"), Trading->VisitState.bPresent);
	TestNull(TEXT("No merchant HUD entry without Cargo Bay"), Trading->VisitState.Merchant.Get());
	return true;
}
#endif
