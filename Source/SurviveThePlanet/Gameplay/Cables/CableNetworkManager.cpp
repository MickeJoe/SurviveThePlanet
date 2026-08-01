#include "Gameplay/Cables/CableNetworkManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Resources/ResourceManager.h"

ACableNetworkManager::ACableNetworkManager()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CableStraightMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/Models/Connectors/StraightConnector.StraightConnector"));
	CableTurnMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/Models/Connectors/CurveConnector.CurveConnector"));
	CableTMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/Models/Connectors/TConnector.TConnector"));
	Cable4Mesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/Models/Connectors/4Connector.4Connector"));
}

void ACableNetworkManager::BeginPlay()
{
	Super::BeginPlay();

	// Recreate transient mesh components when connector topology was restored from a save.
	TArray<FIntPoint> SavedCells;
	CableCells.GetKeys(SavedCells);
	for (const FIntPoint& Cell : SavedCells)
	{
		const uint8 SavedConnections = CableCells.FindChecked(Cell).Connections;
		FSTPCableCell& CableCell = CableCells.FindChecked(Cell);
		CableCell.MeshComponent = nullptr;
		FindOrAddCableCell(Cell);
		CableCell.Connections = SavedConnections;
		RefreshCableCell(Cell);
	}
	RefreshEnergyGrid();
}

void ACableNetworkManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	GridRefreshAccumulator += DeltaSeconds;
	if (GridRefreshAccumulator >= 0.25f)
	{
		GridRefreshAccumulator = 0.0f;
		RefreshEnergyGrid();
	}

	PendingEnergyDelta += (GridProductionPerMinute
		- (bCanSupplyAllConsumers ? GridConsumptionPerMinute : 0.0f)) * DeltaSeconds / 60.0f;
	const int32 WholeEnergyDelta = FMath::TruncToInt(PendingEnergyDelta);
	if (WholeEnergyDelta != 0)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AResourceManager> It(World); It; ++It)
			{
				It->AddResource(EResourceType::Energy, WholeEnergyDelta);
				PendingEnergyDelta -= WholeEnergyDelta;
				break;
			}
		}
	}
}

bool ACableNetworkManager::BeginCableDrag(const FVector& WorldLocation)
{
	FIntPoint Cell;
	if (!TryGetCell(WorldLocation, Cell))
	{
		return false;
	}

	bIsDragging = true;
	DragStartCell = Cell;
	LastDragCell = Cell;
	ConnectionsBeforeDrag.Reset();
	for (const TPair<FIntPoint, FSTPCableCell>& Pair : CableCells)
	{
		ConnectionsBeforeDrag.Add(Pair.Key, Pair.Value.Connections);
	}
	return true;
}

bool ACableNetworkManager::UpdateCableDrag(const FVector& WorldLocation)
{
	if (!bIsDragging)
	{
		return false;
	}

	FIntPoint Cell;
	if (!TryGetCell(WorldLocation, Cell))
	{
		return false;
	}

	if (Cell != LastDragCell)
	{
		RestoreNetworkBeforeDrag();
		AddPathBetweenCells(DragStartCell, Cell);
		LastDragCell = Cell;
	}

	return true;
}

void ACableNetworkManager::EndCableDrag()
{
	const bool bNetworkChanged = bIsDragging;
	bIsDragging = false;
	ConnectionsBeforeDrag.Reset();
	if (bNetworkChanged)
	{
		RefreshEnergyGrid();
		OnCableNetworkChanged.Broadcast();
	}
}

bool ACableNetworkManager::HasConnectorAtCell(FSTPGridCell Cell) const
{
	return CableCells.Contains(Cell.ToIntPoint());
}

void ACableNetworkManager::GetConnectorCells(TArray<FSTPGridCell>& OutCells) const
{
	OutCells.Reset(CableCells.Num());
	for (const TPair<FIntPoint, FSTPCableCell>& Pair : CableCells)
	{
		OutCells.Emplace(Pair.Key);
	}
}

void ACableNetworkManager::GetTouchingCableCells(const ABaseBuilding* Building, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	APlanetSurfaceManager* Manager = const_cast<ACableNetworkManager*>(this)->ResolveSurfaceManager();
	if (!IsValid(Building) || !Manager)
	{
		return;
	}

	const FIntPoint Footprint = Building->GetGridFootprint();
	FSTPGridCell Origin;
	if (!Manager->TryGetActorOriginCell(const_cast<ABaseBuilding*>(Building), Origin))
	{
		// Legacy/special placements (notably mining machines replacing resource
		// actors) may not own the reservation yet. Their snapped transform still
		// gives us an authoritative origin for connector contact testing.
		const FSTPGridPlacement Placement = Manager->GetPlacementForWorldLocation(
			Building->GetActorLocation(), Footprint);
		Origin = Placement.OriginCell;
		if (!Manager->IsCellInBounds(Origin))
		{
			return;
		}
	}

	for (int32 Y = -1; Y <= Footprint.Y; ++Y)
	{
		for (int32 X = -1; X <= Footprint.X; ++X)
		{
			const bool bInsideOrEdge = (X >= 0 && X < Footprint.X && Y >= 0 && Y < Footprint.Y)
				|| ((X == -1 || X == Footprint.X) && Y >= 0 && Y < Footprint.Y)
				|| ((Y == -1 || Y == Footprint.Y) && X >= 0 && X < Footprint.X);
			const FIntPoint Cell(Origin.X + X, Origin.Y + Y);
			if (bInsideOrEdge && CableCells.Contains(Cell))
			{
				OutCells.Add(Cell);
			}
		}
	}
}

bool ACableNetworkManager::IsBuildingConnectedToPowerGrid(const ABaseBuilding* Building) const
{
	if (!IsValid(Building))
	{
		return false;
	}
	if (Building->GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return Building->GetConstructionProgress() >= 1.0f;
	}

	TArray<FIntPoint> StartCells;
	GetTouchingCableCells(Building, StartCells);
	if (StartCells.IsEmpty())
	{
		return false;
	}

	TSet<FIntPoint> HeadquartersNetwork;
	GetHeadquartersNetworkCells(HeadquartersNetwork);
	for (const FIntPoint& Cell : StartCells)
	{
		if (HeadquartersNetwork.Contains(Cell))
		{
			return true;
		}
	}
	return false;
}

void ACableNetworkManager::GetHeadquartersNetworkCells(TSet<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	TArray<FIntPoint> Queue;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABaseBuilding> It(World); It; ++It)
		{
			const ABaseBuilding* Building = *It;
			if (Building->GetBuildingType() == ESTPBuildingType::BaseModule
				&& Building->GetConstructionProgress() >= 1.0f)
			{
				TArray<FIntPoint> HeadquartersCells;
				GetTouchingCableCells(Building, HeadquartersCells);
				Queue.Append(HeadquartersCells);
			}
		}
	}

	for (int32 Index = 0; Index < Queue.Num(); ++Index)
	{
		const FIntPoint Cell = Queue[Index];
		if (OutCells.Contains(Cell))
		{
			continue;
		}
		OutCells.Add(Cell);

		const FSTPCableCell* CableCell = CableCells.Find(Cell);
		if (!CableCell)
		{
			continue;
		}
		if (CableCell->Connections & North) Queue.Add(Cell + FIntPoint(0, 1));
		if (CableCell->Connections & East) Queue.Add(Cell + FIntPoint(1, 0));
		if (CableCell->Connections & South) Queue.Add(Cell + FIntPoint(0, -1));
		if (CableCell->Connections & West) Queue.Add(Cell + FIntPoint(-1, 0));
	}
}

bool ACableNetworkManager::IsBuildingOperational(const ABaseBuilding* Building) const
{
	if (!IsValid(Building) || Building->GetConstructionProgress() < 1.0f)
	{
		return false;
	}
	if (Building->GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return true;
	}
	return IsBuildingConnectedToPowerGrid(Building)
		&& (Building->GetEnergyConsumptionPerMinute() <= 0.0f || bCanSupplyAllConsumers);
}

void ACableNetworkManager::RefreshEnergyGrid()
{
	GridProductionPerMinute = 0.0f;
	GridConsumptionPerMinute = 0.0f;
	GridStorageCapacity = 0.0f;
	AResourceManager* ResourceManager = nullptr;

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			ResourceManager = *It;
			break;
		}

		for (TActorIterator<ABaseBuilding> It(World); It; ++It)
		{
			const ABaseBuilding* Building = *It;
			if (!IsValid(Building) || Building->GetConstructionProgress() < 1.0f)
			{
				continue;
			}

			const bool bOnMainGrid = Building->GetBuildingType() == ESTPBuildingType::BaseModule
				|| IsBuildingConnectedToPowerGrid(Building);
			if (!bOnMainGrid)
			{
				continue;
			}

			GridProductionPerMinute += Building->GetEnergyProductionPerMinute();
			GridConsumptionPerMinute += Building->GetEnergyConsumptionPerMinute();
			GridStorageCapacity += Building->GetEnergyStorageCapacity();
		}
	}

	if (ResourceManager)
	{
		ResourceManager->SetEnergyStorageCapacity(FMath::RoundToInt(GridStorageCapacity));
		bCanSupplyAllConsumers = GridConsumptionPerMinute <= GridProductionPerMinute + KINDA_SMALL_NUMBER
			|| ResourceManager->GetResourceAmount(EResourceType::Energy) > 0;
	}
	else
	{
		bCanSupplyAllConsumers = GridConsumptionPerMinute <= GridProductionPerMinute + KINDA_SMALL_NUMBER;
	}
}

APlanetSurfaceManager* ACableNetworkManager::ResolveSurfaceManager()
{
	if (IsValid(SurfaceManager))
	{
		return SurfaceManager;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
	{
		SurfaceManager = *It;
		break;
	}

	return SurfaceManager;
}

bool ACableNetworkManager::TryGetCell(const FVector& WorldLocation, FIntPoint& OutCell)
{
	APlanetSurfaceManager* Manager = ResolveSurfaceManager();
	if (!Manager)
	{
		return false;
	}

	FSTPGridCell GridCell;
	if (!Manager->GetCellForWorldLocation(WorldLocation, GridCell))
	{
		return false;
	}

	OutCell = GridCell.ToIntPoint();
	return true;
}

void ACableNetworkManager::RestoreNetworkBeforeDrag()
{
	TArray<FIntPoint> CellsToRemove;
	for (TPair<FIntPoint, FSTPCableCell>& Pair : CableCells)
	{
		if (const uint8* PreviousConnections = ConnectionsBeforeDrag.Find(Pair.Key))
		{
			Pair.Value.Connections = *PreviousConnections;
			RefreshCableCell(Pair.Key);
		}
		else
		{
			if (Pair.Value.MeshComponent)
			{
				Pair.Value.MeshComponent->DestroyComponent();
			}
			CellsToRemove.Add(Pair.Key);
		}
	}

	for (const FIntPoint& Cell : CellsToRemove)
	{
		CableCells.Remove(Cell);
	}
}

void ACableNetworkManager::AddPathBetweenCells(const FIntPoint& From, const FIntPoint& To)
{
	FIntPoint Current = From;

	while (Current.X != To.X)
	{
		const FIntPoint Next(Current.X + FMath::Sign(To.X - Current.X), Current.Y);
		ConnectAdjacentCells(Current, Next);
		Current = Next;
	}

	while (Current.Y != To.Y)
	{
		const FIntPoint Next(Current.X, Current.Y + FMath::Sign(To.Y - Current.Y));
		ConnectAdjacentCells(Current, Next);
		Current = Next;
	}
}

void ACableNetworkManager::ConnectAdjacentCells(const FIntPoint& From, const FIntPoint& To)
{
	const FIntPoint Delta = To - From;
	uint8 FromDirection = 0;
	uint8 ToDirection = 0;

	if (Delta == FIntPoint(1, 0))
	{
		FromDirection = East;
		ToDirection = West;
	}
	else if (Delta == FIntPoint(-1, 0))
	{
		FromDirection = West;
		ToDirection = East;
	}
	else if (Delta == FIntPoint(0, 1))
	{
		FromDirection = North;
		ToDirection = South;
	}
	else if (Delta == FIntPoint(0, -1))
	{
		FromDirection = South;
		ToDirection = North;
	}
	else
	{
		return;
	}

	AddConnection(From, FromDirection);
	AddConnection(To, ToDirection);
	RefreshCableCell(From);
	RefreshCableCell(To);
}

void ACableNetworkManager::AddConnection(const FIntPoint& Cell, uint8 Direction)
{
	FSTPCableCell& CableCell = FindOrAddCableCell(Cell);
	CableCell.Connections |= Direction;
}

FSTPCableCell& ACableNetworkManager::FindOrAddCableCell(const FIntPoint& Cell)
{
	if (FSTPCableCell* Existing = CableCells.Find(Cell))
	{
		if (!Existing->MeshComponent)
		{
			Existing->MeshComponent = NewObject<UStaticMeshComponent>(this);
			Existing->MeshComponent->SetupAttachment(SceneRoot);
			Existing->MeshComponent->SetMobility(EComponentMobility::Movable);
			Existing->MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Existing->MeshComponent->RegisterComponent();
			if (APlanetSurfaceManager* Manager = ResolveSurfaceManager())
			{
				Existing->MeshComponent->SetWorldLocation(Manager->GetWorldLocationForCell(FSTPGridCell(Cell))
					+ Manager->GetActorUpVector() * CableHeightOffset);
			}
		}
		return *Existing;
	}

	FSTPCableCell& NewCell = CableCells.Add(Cell);
	NewCell.MeshComponent = NewObject<UStaticMeshComponent>(this);
	NewCell.MeshComponent->SetupAttachment(SceneRoot);
	NewCell.MeshComponent->SetMobility(EComponentMobility::Movable);
	NewCell.MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewCell.MeshComponent->RegisterComponent();

	if (APlanetSurfaceManager* Manager = ResolveSurfaceManager())
	{
		const FVector WorldLocation = Manager->GetWorldLocationForCell(FSTPGridCell(Cell));
		NewCell.MeshComponent->SetWorldLocation(
			WorldLocation + Manager->GetActorUpVector() * CableHeightOffset);
	}

	return NewCell;
}

void ACableNetworkManager::RefreshCableCell(const FIntPoint& Cell)
{
	FSTPCableCell* CableCell = CableCells.Find(Cell);
	if (!CableCell || !CableCell->MeshComponent)
	{
		return;
	}

	UStaticMesh* Mesh = nullptr;
	float LocalPitch = 0.0f;
	float LocalYaw = 0.0f;
	float LocalRoll = 0.0f;
	FVector MeshScale = FVector::OneVector;
	const uint8 Connections = CableCell->Connections;

	if (Connections == (North | South))
	{
		Mesh = CableStraightMesh;
		LocalPitch = StraightBasePitch;
		LocalYaw = StraightBaseYaw;
		LocalRoll = StraightBaseRoll;
		MeshScale = StraightMeshScale;
	}
	else if (Connections == (East | West))
	{
		Mesh = CableStraightMesh;
		LocalPitch = StraightBasePitch;
		LocalYaw = StraightBaseYaw + 90.0f;
		LocalRoll = StraightBaseRoll;
		MeshScale = StraightMeshScale;
	}
	else if (Connections == (North | East))
	{
		Mesh = CableTurnMesh;
		LocalYaw = TurnBaseYaw + 270.0f;
		MeshScale = TurnMeshScale;
	}
	else if (Connections == (East | South))
	{
		Mesh = CableTurnMesh;
		LocalYaw = TurnBaseYaw + 180.0f;
		MeshScale = TurnMeshScale;
	}
	else if (Connections == (South | West))
	{
		Mesh = CableTurnMesh;
		LocalYaw = TurnBaseYaw + 90.0f;
		MeshScale = TurnMeshScale;
	}
	else if (Connections == (West | North))
	{
		Mesh = CableTurnMesh;
		LocalYaw = TurnBaseYaw;
		MeshScale = TurnMeshScale;
	}
	else if (Connections == (North | East | West))
	{
		Mesh = CableTMesh;
		LocalYaw = TBaseYaw;
	}
	else if (Connections == (North | East | South))
	{
		Mesh = CableTMesh;
		LocalYaw = TBaseYaw + 270.0f;
	}
	else if (Connections == (East | South | West))
	{
		Mesh = CableTMesh;
		LocalYaw = TBaseYaw + 180.0f;
	}
	else if (Connections == (North | South | West))
	{
		Mesh = CableTMesh;
		LocalYaw = TBaseYaw + 90.0f;
	}
	else if (Connections == (North | East | South | West))
	{
		Mesh = Cable4Mesh;
		LocalYaw = FourWayBaseYaw;
	}
	else if ((Connections & (North | South)) != 0)
	{
		Mesh = CableStraightMesh;
		LocalPitch = StraightBasePitch;
		LocalYaw = StraightBaseYaw;
		LocalRoll = StraightBaseRoll;
		MeshScale = StraightMeshScale;
	}
	else if ((Connections & (East | West)) != 0)
	{
		Mesh = CableStraightMesh;
		LocalPitch = StraightBasePitch;
		LocalYaw = StraightBaseYaw + 90.0f;
		LocalRoll = StraightBaseRoll;
		MeshScale = StraightMeshScale;
	}

	CableCell->MeshComponent->SetStaticMesh(Mesh);
	CableCell->MeshComponent->SetWorldScale3D(MeshScale);
	if (APlanetSurfaceManager* Manager = ResolveSurfaceManager())
	{
		CableCell->MeshComponent->SetWorldRotation(
			Manager->GetActorRotation() + FRotator(LocalPitch, LocalYaw, LocalRoll));
	}
}
