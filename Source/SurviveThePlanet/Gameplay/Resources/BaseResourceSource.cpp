#include "BaseResourceSource.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Gameplay/Buildings/MiningMachine.h"

ABaseResourceSource::ABaseResourceSource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	SetRootComponent(ResourceMesh);
	// Mining placement uses a complex Visibility trace under the cursor. Resource
	// meshes must win that trace instead of letting it pass through to the planet.
	ResourceMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ResourceMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ResourceMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ResourceMesh->SetGenerateOverlapEvents(false);
}

void ABaseResourceSource::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	StartingAmount = FMath::Max(0, StartingAmount);
	RemainingAmount = StartingAmount;
	RefreshGridCell();
}

void ABaseResourceSource::BeginPlay()
{
	Super::BeginPlay();

	StartingAmount = FMath::Max(0, StartingAmount);
	RemainingAmount = StartingAmount;
	RefreshGridCell();

	if (SurfaceManager)
	{
		SurfaceManager->ReserveCells(this, GridCell, GetGridFootprint());
	}
}

void ABaseResourceSource::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SurfaceManager)
	{
		SurfaceManager->ReleaseCells(this);
	}

	Super::EndPlay(EndPlayReason);
}

int32 ABaseResourceSource::ExtractResource(int32 RequestedAmount)
{
	if (RequestedAmount <= 0 || RemainingAmount <= 0)
	{
		return 0;
	}

	const int32 ExtractedAmount = FMath::Min(RequestedAmount, RemainingAmount);
	RemainingAmount -= ExtractedAmount;
	OnRemainingAmountChanged.Broadcast(RemainingAmount);

	if (RemainingAmount == 0)
	{
		OnDepleted.Broadcast();
	}

	return ExtractedAmount;
}

bool ABaseResourceSource::TryReserveMiningMachine(AMiningMachine* MiningMachine)
{
	if (!IsValid(MiningMachine))
	{
		return false;
	}

	if (IsValid(ReservedMiningMachine) && ReservedMiningMachine != MiningMachine)
	{
		return false;
	}

	ReservedMiningMachine = MiningMachine;
	RefreshResourceMeshVisibility();
	return true;
}

void ABaseResourceSource::ReleaseMiningMachine(AMiningMachine* MiningMachine)
{
	if (ReservedMiningMachine == MiningMachine)
	{
		ReservedMiningMachine = nullptr;
		RefreshResourceMeshVisibility();
	}
}

bool ABaseResourceSource::IsReservedForMining() const
{
	return IsValid(ReservedMiningMachine);
}

void ABaseResourceSource::SetPreviewingMiningMachine(AMiningMachine* MiningMachine)
{
	PreviewingMiningMachine = MiningMachine;
	RefreshResourceMeshVisibility();
}

void ABaseResourceSource::RefreshResourceMeshVisibility()
{
	if (ResourceMesh)
	{
		ResourceMesh->SetHiddenInGame(IsValid(ReservedMiningMachine) || IsValid(PreviewingMiningMachine));
	}
}

void ABaseResourceSource::RefreshGridCell()
{
	SurfaceManager = FindSurfaceManager();
	if (SurfaceManager)
	{
		const FSTPGridPlacement Placement = SurfaceManager->GetPlacementForWorldLocation(GetActorLocation(), GetGridFootprint());
		GridCell = Placement.OriginCell;
	}
}

FIntPoint ABaseResourceSource::GetGridFootprint() const
{
	if (!ResourceMesh || !ResourceMesh->GetStaticMesh())
	{
		return FIntPoint(1, 1);
	}

	const FVector Size = ResourceMesh->GetStaticMesh()->GetBoundingBox().GetSize()
		* ResourceMesh->GetRelativeScale3D().GetAbs();
	const float CellSize = SurfaceManager ? FMath::Max(1.0f, SurfaceManager->GetTileSpacing()) : 100.0f;
	return FIntPoint(
		FMath::Max(1, FMath::CeilToInt(Size.X / CellSize)),
		FMath::Max(1, FMath::CeilToInt(Size.Y / CellSize)));
}

APlanetSurfaceManager* ABaseResourceSource::FindSurfaceManager() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}
