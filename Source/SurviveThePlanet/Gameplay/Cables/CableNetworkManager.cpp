#include "Gameplay/Cables/CableNetworkManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"

ACableNetworkManager::ACableNetworkManager()
{
	PrimaryActorTick.bCanEverTick = false;
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
	bIsDragging = false;
	ConnectionsBeforeDrag.Reset();
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
