#include "Gameplay/Buildings/RemoteBase.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "UObject/ConstructorHelpers.h"

ARemoteBase::ARemoteBase()
{
	BuildingType = ESTPBuildingType::RemoteBase;
	BuildingTag = TEXT("RemoteBase");
	BuildingDisplayName = NSLOCTEXT("STP", "RemoteBaseName", "Remote Base");
	BuildingDescription = NSLOCTEXT("STP", "RemoteBaseDescription", "Establishes a discovered sector for construction. Connectors are charged by distance to the nearest power connection.");
	ConstructionProgress = 0.0f;
	PowerConnection = CreateDefaultSubobject<UEnergyConnectionComponent>(TEXT("PowerConnection"));
	PowerConnection->SetupAttachment(SceneRoot);
	PowerConnection->SourceSwitchMargin = 0.0f;
	EnergyCoverage = CreateDefaultSubobject<UEnergyCoverageComponent>(TEXT("EnergyCoverage"));
	EnergyCoverage->SetupAttachment(SceneRoot);
	EnergyCoverage->CoverageRadius = 2200.0f;
	Antenna = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Antenna"));
	Perimeter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Perimeter"));
	Scan = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scan"));
	for (UStaticMeshComponent* Part : {Antenna.Get(), Perimeter.Get(), Scan.Get()})
	{
		Part->SetupAttachment(BuildingMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
	}
	Antenna->SetRelativeLocation(FVector(0, 0, 218));
	Perimeter->SetCastShadow(false);
	Scan->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Game/Units/Buildings/RemoteBase/Meshes/SM_RemoteBase_Body.SM_RemoteBase_Body"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> AntennaMesh(TEXT("/Game/Units/Buildings/RemoteBase/Meshes/SM_RemoteBase_Antenna.SM_RemoteBase_Antenna"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> RingMesh(TEXT("/Game/Units/Buildings/RemoteBase/Meshes/SM_RemoteBase_Ring.SM_RemoteBase_Ring"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ScanMesh(TEXT("/Game/Units/Buildings/RemoteBase/Meshes/SM_RemoteBase_Scan.SM_RemoteBase_Scan"));
	BuildingMesh->SetStaticMesh(BodyMesh.Object);
	BuildingMesh->SetRelativeLocation(FVector(-16.31f, -37.45f, 0));
	Antenna->SetStaticMesh(AntennaMesh.Object);
	Perimeter->SetStaticMesh(RingMesh.Object);
	Scan->SetStaticMesh(ScanMesh.Object);
}

void ARemoteBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GroundBuildingMesh(true);
	RefreshVisuals();
}

void ARemoteBase::BeginPlay()
{
	Super::BeginPlay();
	GroundBuildingMesh(true);
	RefreshSector();
	RefreshVisuals();
}

void ARemoteBase::SetConstructionProgress(float NewProgress)
{
	Super::SetConstructionProgress(NewProgress);
	RefreshSector();
	RefreshVisuals();
}

void ARemoteBase::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	RefreshVisuals();
}

void ARemoteBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	GroundBuildingMesh();
	RefreshSector();
	RefreshVisuals();
	if (IsOperational()) Antenna->AddLocalRotation(FRotator(0, 12.0f * DeltaSeconds, 0));
}

void ARemoteBase::RefreshVisuals()
{
	const bool bActive = !IsPlacementPreview() && IsOperational();
	Perimeter->SetVisibility(bActive);
	Scan->SetVisibility(bActive);
}

void ARemoteBase::RefreshSector()
{
	if (!GetWorld() || !GetWorld()->IsGameWorld() || GetWorld()->bIsTearingDown) return;
	if (!SectorGrid.IsValid())
	{
		for (TActorIterator<AHexSectorGrid> It(GetWorld()); It; ++It) { SectorGrid = *It; break; }
	}
	if (!SectorGrid.IsValid()) return;
	if (IsPlacementPreview() || GetConstructionProgress() < 1.0f)
	{
		if (EstablishedSectorId != INDEX_NONE && !SectorGrid->HasSectorBase(EstablishedSectorId, this))
			SectorGrid->SetSectorState(EstablishedSectorId, ESectorState::Discovered);
		EstablishedSectorId = INDEX_NONE;
		return;
	}
	const int32 Id = SectorGrid->GetSectorAtWorldLocation(GetActorLocation());
	FHexSector Sector;
	if (SectorGrid->GetSectorById(Id, Sector) && Sector.State != ESectorState::Undiscovered)
	{
		SectorGrid->SetSectorState(Id, ESectorState::Established);
		EstablishedSectorId = Id;
	}
}

void ARemoteBase::ReleaseSector()
{
	if (GetWorld() && !GetWorld()->bIsTearingDown && SectorGrid.IsValid()
		&& EstablishedSectorId != INDEX_NONE && !SectorGrid->HasSectorBase(EstablishedSectorId, this))
	{
		SectorGrid->SetSectorState(EstablishedSectorId, ESectorState::Discovered);
	}
	EstablishedSectorId = INDEX_NONE;
}

void ARemoteBase::Destroyed()
{
	ReleaseSector();
	Super::Destroyed();
}

void ARemoteBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseSector();
	Super::EndPlay(EndPlayReason);
}
