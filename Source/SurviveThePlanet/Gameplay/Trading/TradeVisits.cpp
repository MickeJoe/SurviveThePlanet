#include "Gameplay/Trading/TradeSubsystem.h"

#include "EngineUtils.h"
#include "Gameplay/Trading/MerchantShip.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Planet/PlanetDefinition.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Trading/TraderSubsystem.h"
#include "Kismet/GameplayStatics.h"

TStatId UTradeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTradeSubsystem, STATGROUP_Tickables);
}

void UTradeSubsystem::ResolveCargoBay()
{
	if (!Planet)
	{
		for (TActorIterator<APlanetWeatherManager> It(GetWorld()); It; ++It)
		{
			if (It->GetPlanetDefinition())
			{
				Planet = It->GetPlanetDefinition();
				break;
			}
		}
	}
	if (DockCargoBay.IsValid() && !DockCargoBay->IsPlacementPreview()
		&& DockCargoBay->GetConstructionProgress() >= 1.0f)
	{
		bCargoBayReady = true;
		return;
	}
	DockCargoBay.Reset();
	bCargoBayReady = false;
	for (TActorIterator<ACargoBay> It(GetWorld()); It; ++It)
	{
		if (!It->IsPlacementPreview() && It->GetConstructionProgress() >= 1.0f)
		{
			bCargoBayReady = true;
			DockCargoBay = *It;
			break;
		}
	}
}

void UTradeSubsystem::Tick(float DeltaTime)
{
	CargoCheckElapsed += DeltaTime;
	if (CargoCheckElapsed >= 0.5f || !Planet)
	{
		CargoCheckElapsed = 0.0f;
		ResolveCargoBay();
	}
	if (bCargoBayReady && UGameplayStatics::GetGlobalTimeDilation(this) >= 0.001f)
	{
		// The colony clock advances one game minute per simulation second.
		MinutesSinceCargoBay += FMath::Max(0.0f, DeltaTime);
	}
	RefreshVisitState();
	UpdateMerchantShip();
}

UMerchantDefinition* UTradeSubsystem::GetMerchant(FName TraderId)
{
	if (!Planet) ResolveCargoBay();
	if (Planet)
	{
		for (const FMerchantScheduledVisit& Visit : Planet->MerchantVisits.Visits)
		{
			if (Visit.Merchant && Visit.Merchant->Id == TraderId) return Visit.Merchant;
		}
	}
	return nullptr;
}

FMerchantVisitState UTradeSubsystem::GetVisitState()
{
	// Also detect completion while the simulation is paused.
	ResolveCargoBay();
	RefreshVisitState();
	return VisitState;
}

bool UTradeSubsystem::IsTraderAvailable(FName TraderId)
{
	if (GetMerchant(TraderId))
	{
		const FMerchantVisitState State = GetVisitState();
		return State.bCargoBayReady && State.bPresent && State.Merchant && State.Merchant->Id == TraderId
			&& (!State.Merchant->ShipMesh || State.bShipDocked);
	}
#if !UE_BUILD_SHIPPING
	// Retain the I-menu market for development without creating a faction visit.
	FSTPTraderDefinition Trader;
	return GetWorld()->GetSubsystem<UTraderSubsystem>()->GetTrader(TraderId, Trader);
#else
	return false;
#endif
}

void UTradeSubsystem::RefreshVisitState()
{
	const FName PreviousId = VisitState.bPresent && VisitState.Merchant ? VisitState.Merchant->Id : NAME_None;
	VisitState = FMerchantVisitState();
	VisitState.bCargoBayReady = bCargoBayReady;
	if (!Planet || !bCargoBayReady) return;
	const FMerchantVisitSchedule& Schedule = Planet->MerchantVisits;
	const double CycleMinutes = FMath::Max(0.1f, Schedule.RepeatCycleDays) * 1440.0;
	const double CycleStart = FMath::FloorToDouble(MinutesSinceCargoBay / CycleMinutes) * CycleMinutes;
	double NextArrival = TNumericLimits<double>::Max();
	double PreviousArrival = 0.0;
	double PresentArrival = -1.0;
	UMerchantDefinition* NextMerchant = nullptr;
	for (const FMerchantScheduledVisit& Visit : Schedule.Visits)
	{
		if (!Visit.Merchant || Visit.Merchant->Id.IsNone() || Visit.ArrivalAfterHours < 0.0f) continue;
		const double Offset = Visit.ArrivalAfterHours * 60.0;
		if (Offset >= CycleMinutes) continue;
		const double Stay = FMath::Clamp(static_cast<double>(Visit.StayHours) * 60.0, 0.0, CycleMinutes);
		for (int32 CycleOffset = -1; CycleOffset <= 1; ++CycleOffset)
		{
			const double Arrival = CycleStart + CycleOffset * CycleMinutes + Offset;
			if (Arrival < 0.0) continue;
			if (Arrival <= MinutesSinceCargoBay)
			{
				PreviousArrival = FMath::Max(PreviousArrival, Arrival);
				if (MinutesSinceCargoBay < Arrival + Stay && Arrival > PresentArrival)
				{
					PresentArrival = Arrival;
					VisitState.Merchant = Visit.Merchant;
					VisitState.bPresent = true;
					VisitState.RemainingGameMinutes = Arrival + Stay - MinutesSinceCargoBay;
				}
			}
			else if (Arrival < NextArrival)
			{
				NextArrival = Arrival;
				NextMerchant = Visit.Merchant;
			}
		}
	}
	if (VisitState.bPresent)
	{
		VisitState.ArrivalProgress = 1.0f;
		VisitState.bShipDocked = IsValid(MerchantShip) && MerchantShip->IsDocked();
		if (PreviousId != VisitState.Merchant->Id || ActiveArrivalMinute != PresentArrival)
		{
			// A returning merchant brings fresh stock; sales do not persist between visits.
			TraderStock.Remove(VisitState.Merchant->Id);
			ActiveArrivalMinute = PresentArrival;
		}
	}
	else if (NextMerchant)
	{
		VisitState.Merchant = NextMerchant;
		VisitState.RemainingGameMinutes = NextArrival - MinutesSinceCargoBay;
		VisitState.ArrivalProgress = FMath::Clamp(
			static_cast<float>((MinutesSinceCargoBay - PreviousArrival) / (NextArrival - PreviousArrival)), 0.0f, 1.0f);
	}
}

void UTradeSubsystem::UpdateMerchantShip()
{
	UMerchantDefinition* PresentMerchant = VisitState.bPresent ? VisitState.Merchant.Get() : nullptr;
	if (IsValid(MerchantShip) && (!PresentMerchant || MerchantShip->GetMerchant() != PresentMerchant))
	{
		MerchantShip->Depart();
		MerchantShip = nullptr;
	}
	if (PresentMerchant && PresentMerchant->ShipMesh && DockCargoBay.IsValid() && !IsValid(MerchantShip))
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		MerchantShip = GetWorld()->SpawnActor<AMerchantShip>(AMerchantShip::StaticClass(),
			DockCargoBay->GetMerchantDockTransform(), Parameters);
		if (MerchantShip) MerchantShip->Arrive(PresentMerchant, DockCargoBay.Get());
	}
}
