#include "BaseResourceSource.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

ABaseResourceSource::ABaseResourceSource()
{
	PrimaryActorTick.bCanEverTick = false;

	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	SetRootComponent(ResourceMesh);
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
		SurfaceManager->ReserveCells(this, GridCell, FIntPoint(1, 1));
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

void ABaseResourceSource::RefreshGridCell()
{
	SurfaceManager = FindSurfaceManager();
	if (SurfaceManager)
	{
		SurfaceManager->GetCellForWorldLocation(GetActorLocation(), GridCell);
	}
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
