#include "Gameplay/Trading/TradeSubsystem.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Trading/TraderSubsystem.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Planet/PlanetDefinition.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
bool CombineLines(const TArray<FTradeLine>& Lines, TMap<FName, int32>& Combined)
{
	for (const FTradeLine& Line : Lines)
	{
		if (Line.ItemId.IsNone() || Line.Quantity <= 0)
		{
			return false;
		}
		int32& Quantity = Combined.FindOrAdd(Line.ItemId);
		if (Line.Quantity > MAX_int32 - Quantity)
		{
			return false;
		}
		Quantity += Line.Quantity;
	}
	return true;
}
} // namespace

bool UTradeSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

UTradeCatalog* UTradeSubsystem::GetCatalog()
{
	if (!Catalog)
	{
		Catalog = LoadObject<UTradeCatalog>(nullptr, TEXT("/Game/Data/Trading/DA_TradeCatalog.DA_TradeCatalog"));
	}
	return Catalog;
}

TArray<FTradeItemDefinition> UTradeSubsystem::GetItems()
{
	return GetCatalog() ? Catalog->Items : TArray<FTradeItemDefinition>();
}

const FTradeItemDefinition* UTradeSubsystem::FindItem(FName Id)
{
	return GetCatalog()
			   ? Catalog->Items.FindByPredicate([Id](const FTradeItemDefinition& Item) { return Item.Id == Id; })
			   : nullptr;
}

AResourceManager* UTradeSubsystem::FindResourceManager() const
{
	for (TActorIterator<AResourceManager> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

int32 UTradeSubsystem::GetStock(FName TraderId, FName ItemId)
{
	const FTradeItemDefinition* Item = FindItem(ItemId);
	if (!Item)
	{
		return 0;
	}

	if (const UMerchantDefinition* Merchant = GetMerchant(TraderId))
	{
		if (!IsTraderAvailable(TraderId) ||
			(!Merchant->OfferedItemIds.IsEmpty() && !Merchant->OfferedItemIds.Contains(ItemId)))
		{
			return 0;
		}
	}
	TMap<FName, int32>& Stock = TraderStock.FindOrAdd(TraderId);
	if (!Stock.Contains(ItemId))
	{
		Stock.Add(ItemId, FMath::Max(0, Item->StartingStock));
	}
	return Stock.FindRef(ItemId);
}

TArray<ABaseDrone*> UTradeSubsystem::FindDrones(const FTradeItemDefinition& Item, bool bSellableOnly) const
{
	TArray<ABaseDrone*> Result;
	for (TActorIterator<ABaseDrone> It(GetWorld()); It; ++It)
	{
		if (It->GetClass() == Item.DroneClass &&
			(!bSellableOnly || (It->IsAvailableForAssignment() && !It->IsMovingAsideForConstruction())))
		{
			Result.Add(*It);
		}
	}
	return Result;
}

int32 UTradeSubsystem::GetOwnedCount(FName ItemId)
{
	const FTradeItemDefinition* Item = FindItem(ItemId);
	if (!Item)
	{
		return 0;
	}
	if (Item->Kind == ETradeItemKind::Resource)
	{
		const AResourceManager* Manager = FindResourceManager();
		return Manager ? Manager->GetResourceAmount(Item->Resource) : 0;
	}
	if (Item->Kind == ETradeItemKind::Drone)
	{
		return FindDrones(*Item, false).Num();
	}
	const UBuildingBlueprintSubsystem* Inventory =
		GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>()
									  : nullptr;
	return Inventory && Inventory->OwnsBlueprint(Item->BuildTool) ? 1 : 0;
}

int32 UTradeSubsystem::GetSellableCount(FName ItemId)
{
	const FTradeItemDefinition* Item = FindItem(ItemId);
	if (!Item || Item->SellPrice <= 0 || Item->Kind == ETradeItemKind::Blueprint)
	{
		return 0;
	}
	return Item->Kind == ETradeItemKind::Drone ? FindDrones(*Item, true).Num() : GetOwnedCount(ItemId);
}

FTradeQuote UTradeSubsystem::QuoteTrade(FName TraderId, const TArray<FTradeLine>& Buying,
										const TArray<FTradeLine>& Selling)
{
	FTradeQuote Quote;
	AResourceManager* Manager = FindResourceManager();
	Quote.BalanceAfter = Manager ? Manager->GetCredits() : 0;

	TMap<FName, int32> Buy, Sell;
	if (!Manager || !GetCatalog() || !IsTraderAvailable(TraderId) || !CombineLines(Buying, Buy) ||
		!CombineLines(Selling, Sell))
	{
		Quote.Status = FText::FromString(TEXT("This trade is unavailable."));
		return Quote;
	}
	int64 Purchase = 0, Sale = 0;
	for (const auto& Pair : Buy)
	{
		const FTradeItemDefinition* Item = FindItem(Pair.Key);
		if (!Item || Item->BuyPrice <= 0 || Pair.Value > GetStock(TraderId, Pair.Key) || Sell.Contains(Pair.Key) ||
			(Item->Kind == ETradeItemKind::Blueprint && (GetOwnedCount(Pair.Key) > 0 || Pair.Value != 1)) ||
			(Item->Kind == ETradeItemKind::Drone && (!Item->DroneClass || Pair.Value > 20)))
		{
			Quote.Status = FText::FromString(TEXT("Check trader stock and selected quantities."));
			return Quote;
		}
		if (Item->Kind == ETradeItemKind::Blueprint)
		{
			const UBuildingManagerSubsystem* Buildings = GetWorld()->GetSubsystem<UBuildingManagerSubsystem>();
			if (!Buildings || !Buildings->GetDefinition(Item->BuildTool) || !GetWorld()->GetGameInstance() ||
				!GetWorld()->GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>())
			{
				Quote.Status = FText::FromString(TEXT("This blueprint is unavailable."));
				return Quote;
			}
		}
		if (Item->Kind == ETradeItemKind::Resource && Pair.Value > MAX_int32 - GetOwnedCount(Pair.Key))
		{
			Quote.Status = FText::FromString(TEXT("Inventory capacity exceeded."));
			return Quote;
		}
		Purchase += static_cast<int64>(Item->BuyPrice) * Pair.Value;
		if (Purchase > MAX_int32)
		{
			break;
		}
	}
	for (const auto& Pair : Sell)
	{
		const FTradeItemDefinition* Item = FindItem(Pair.Key);
		if (!Item || Pair.Value > GetSellableCount(Pair.Key) || Pair.Value > MAX_int32 - GetStock(TraderId, Pair.Key))
		{
			Quote.Status = FText::FromString(TEXT("Some selected items are no longer available to sell."));
			return Quote;
		}
		Sale += static_cast<int64>(Item->SellPrice) * Pair.Value;
		if (Sale > MAX_int32)
		{
			break;
		}
	}
	const int64 Balance = static_cast<int64>(Manager->GetCredits()) + Sale - Purchase;
	if (Purchase > MAX_int32 || Sale > MAX_int32 || Balance > MAX_int32)
	{
		Quote.Status = FText::FromString(TEXT("Trade total exceeds the supported balance."));
		return Quote;
	}
	Quote.PurchaseTotal = static_cast<int32>(Purchase);
	Quote.SaleTotal = static_cast<int32>(Sale);
	Quote.BalanceAfter = static_cast<int32>(FMath::Max<int64>(0, Balance));
	Quote.bCanConfirm = Balance >= 0 && (!Buy.IsEmpty() || !Sell.IsEmpty());
	Quote.Status = Balance < 0
					   ? FText::FromString(TEXT("Not enough credits. Sell items or reduce your purchase."))
					   : FText::FromString(Quote.bCanConfirm ? TEXT("Review your items, then confirm the whole trade.")
															 : TEXT("Choose items to buy or sell."));
	return Quote;
}

bool UTradeSubsystem::PrepareDroneDeliveries(const TArray<FTradeLine>& Buying, TArray<ABaseDrone*>& OutDrones)
{
	APlanetSurfaceManager* Surface = nullptr;
	ABaseBuilding* Base = nullptr;
	for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
	{
		Surface = *It;
		break;
	}
	for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
	{
		if (It->GetBuildingType() == ESTPBuildingType::BaseModule)
		{
			Base = *It;
			break;
		}
	}
	for (const FTradeLine& Line : Buying)
	{
		const FTradeItemDefinition* Item = FindItem(Line.ItemId);
		if (!Item || Item->Kind != ETradeItemKind::Drone)
		{
			continue;
		}
		if (!Surface || !Base)
		{
			return false;
		}
		for (int32 Index = 0; Index < Line.Quantity; ++Index)
		{
			const ABaseDrone* Defaults = Item->DroneClass->GetDefaultObject<ABaseDrone>();
			FSTPGridCell Cell;
			FVector Location;
			if (!Surface->FindNearestFreeCellAdjacentToActor(Base, Defaults->GetGridFootprint(), Cell, Location))
			{
				return false;
			}
			const FSTPGridPlacement Placement =
				Surface->GetPlacementForWorldLocation(Location, Defaults->GetGridFootprint());
			if (!Placement.bValid || !Surface->HasTerrainClearance(Placement.OriginCell, Defaults->GetGridFootprint()))
			{
				return false;
			}
			ABaseDrone* Drone = GetWorld()->SpawnActorDeferred<ABaseDrone>(
				Item->DroneClass, FTransform(Placement.WorldRotation, Placement.WorldLocation), nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Drone)
			{
				return false;
			}
			OutDrones.Add(Drone);
			if (!Surface->ReserveCells(Drone, Placement.OriginCell, Drone->GetGridFootprint()))
			{
				return false;
			}
		}
	}
	return true;
}

void UTradeSubsystem::RollbackDroneDeliveries(const TArray<ABaseDrone*>& Drones)
{
	for (ABaseDrone* Drone : Drones)
	{
		for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
		{
			It->ReleaseCells(Drone);
		}
		if (IsValid(Drone))
		{
			Drone->Destroy();
		}
	}
}

bool UTradeSubsystem::ConfirmTrade(FName TraderId, const TArray<FTradeLine>& Buying, const TArray<FTradeLine>& Selling,
								   FText& OutStatus)
{
	if (bCommitting)
	{
		return false;
	}
	const FTradeQuote Quote = QuoteTrade(TraderId, Buying, Selling);
	OutStatus = Quote.Status;
	if (!Quote.bCanConfirm)
	{
		return false;
	}
	TGuardValue<bool> CommitGuard(bCommitting, true);
	TArray<ABaseDrone*> Deliveries;
	if (!PrepareDroneDeliveries(Buying, Deliveries))
	{
		RollbackDroneDeliveries(Deliveries);
		OutStatus = FText::FromString(TEXT("No clear delivery space beside the base. Nothing was traded."));
		return false;
	}
	AResourceManager* Manager = FindResourceManager();
	UBuildingBlueprintSubsystem* Blueprints =
		GetWorld()->GetGameInstance() ? GetWorld()->GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>()
									  : nullptr;
	for (const FTradeLine& Line : Selling)
	{
		const FTradeItemDefinition& Item = *FindItem(Line.ItemId);
		if (Item.Kind == ETradeItemKind::Resource)
		{
			Manager->AddResource(Item.Resource, -Line.Quantity);
		}
		else
		{
			const TArray<ABaseDrone*> Drones = FindDrones(Item, true);
			for (int32 Index = 0; Index < Line.Quantity; ++Index)
			{
				for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
				{
					It->ReleaseCells(Drones[Index]);
				}
				Drones[Index]->Destroy();
			}
		}
		TraderStock.FindOrAdd(TraderId).FindOrAdd(Item.Id) += Line.Quantity;
	}
	for (const FTradeLine& Line : Buying)
	{
		const FTradeItemDefinition& Item = *FindItem(Line.ItemId);
		if (Item.Kind == ETradeItemKind::Resource)
		{
			Manager->AddResource(Item.Resource, Line.Quantity);
		}
		else if (Item.Kind == ETradeItemKind::Blueprint)
		{
			Blueprints->GrantBlueprint(Item.BuildTool);
		}
		TraderStock.FindOrAdd(TraderId).FindOrAdd(Item.Id) -= Line.Quantity;
	}
	Manager->SetCredits(Quote.BalanceAfter);
	for (ABaseDrone* Drone : Deliveries)
	{
		const FTransform Transform = Drone->GetActorTransform();
		for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
		{
			It->ReleaseCells(Drone);
		}
		UGameplayStatics::FinishSpawningActor(Drone, Transform);
	}
	OutStatus = FText::FromString(TEXT("Trade completed. Purchased items have been delivered."));
	OnTradeCompleted.Broadcast();
	return true;
}
