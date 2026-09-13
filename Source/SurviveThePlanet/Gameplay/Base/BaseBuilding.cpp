#include "Gameplay/Base/BaseBuilding.h"

#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/UI/ConstructionProgressBarWidget.h"

ABaseBuilding::ABaseBuilding()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(SceneRoot);

	ConstructionProgressBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConstructionProgressBar"));
	ConstructionProgressBar->SetupAttachment(SceneRoot);
	ConstructionProgressBar->SetWidgetClass(UConstructionProgressBarWidget::StaticClass());
	ConstructionProgressBar->SetWidgetSpace(EWidgetSpace::Screen);
	ConstructionProgressBar->SetDrawSize(FVector2D(120.0f, 14.0f));
	ConstructionProgressBar->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	ConstructionProgressBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ConstructionProgressBar->SetHiddenInGame(false);

	ConfigureMesh();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ghost(TEXT("/Game/UI/Materials/M_BuildPlacement.M_BuildPlacement"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	PlacementMaterial = Ghost.Object;
	PlacementLineMesh = Cube.Object;
}

void ABaseBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Blueprints may configure the inherited BuildingMesh component directly.
	// Only apply the optional base-class override when one was explicitly set;
	// assigning nullptr here would erase the Blueprint component template mesh.
	UStaticMesh* ConfiguredMesh = IsValid(BuildingData) ? BuildingData->BuildingMesh.Get() : BaseModuleMesh.Get();
	if (BuildingMesh && ConfiguredMesh)
	{
		BuildingMesh->SetStaticMesh(ConfiguredMesh);
	}
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	// Resolve catalog data only after the world has initialized. Loading a data
	// asset from a native actor constructor can recurse while its Blueprint class
	// is being created (catalog -> data asset -> Blueprint CDO -> data asset).
	if (!IsValid(BuildingData))
	{
		ESTPBuildTool BuildTool = ESTPBuildTool::None;
		switch (BuildingType)
		{
		case ESTPBuildingType::EnergyModule: BuildTool = ESTPBuildTool::EnergyModule; break;
		case ESTPBuildingType::EnergyStorage: BuildTool = ESTPBuildTool::EnergyStorage; break;
		case ESTPBuildingType::MiningMachine: BuildTool = ESTPBuildTool::MiningMachine; break;
		case ESTPBuildingType::WaterCollector: BuildTool = ESTPBuildTool::WaterCollector; break;
		case ESTPBuildingType::ConcretePlant: BuildTool = ESTPBuildTool::ConcretePlant; break;
		case ESTPBuildingType::CommunicationModule: BuildTool = ESTPBuildTool::CommunicationModule; break;
		case ESTPBuildingType::CargoBay: BuildTool = ESTPBuildTool::CargoBay; break;
		case ESTPBuildingType::Steelworks: BuildTool = ESTPBuildTool::Steelworks; break;
		default: break;
		}

		if (BuildTool != ESTPBuildTool::None)
		{
			if (UBuildingManagerSubsystem* Manager = GetWorld()->GetSubsystem<UBuildingManagerSubsystem>())
			{
				BuildingData = Manager->GetDefinition(BuildTool);
			}
		}
	}

	CurrentHealth = GetMaxHealth();
	const int32 SlotCapacity = GetMaxDroneSlots();
	UnlockedDroneSlots = FMath::Clamp(GetInitiallyUnlockedDroneSlots(), 0, SlotCapacity);
	AssignedDrones.SetNum(SlotCapacity);

	const FName ConfiguredTag = GetBuildingTag();
	if (!ConfiguredTag.IsNone())
	{
		Tags.AddUnique(ConfiguredTag);
	}

	RefreshConstructionProgressBar();
}

bool ABaseBuilding::IsConnectedToPowerGrid() const
{
	if (GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return GetConstructionProgress() >= 1.0f;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			return It->IsBuildingConnectedToPowerGrid(this);
		}
	}
	return false;
}

bool ABaseBuilding::IsOperational() const
{
	if (GetBuildingType() == ESTPBuildingType::BaseModule)
	{
		return true;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
		{
			return It->IsBuildingOperational(this);
		}
	}
	return false;
}

bool ABaseBuilding::ProvidesVision() const
{
	const bool bIsRevealSource = GetBuildingType() == ESTPBuildingType::BaseModule || bProvidesVision;
	return bIsRevealSource && GetVisionRadius() > 0.0f && GetConstructionProgress() >= 1.0f && IsOperational();
}

float ABaseBuilding::GetEnergyConsumptionPerMinute() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyConsumptionPerMinute
		: EnergyConsumptionPerMinute;
}

float ABaseBuilding::GetEnergyProductionPerMinute() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyProductionPerMinute
		: EnergyProductionPerMinute;
}

float ABaseBuilding::GetEnergyStorageCapacity() const
{
	return IsValid(BuildingData) && BuildingData->bOverrideEnergySettings
		? BuildingData->EnergyStorageCapacity
		: EnergyStorageCapacity;
}

ABaseDrone* ABaseBuilding::GetAssignedDroneAtSlot(int32 SlotIndex) const
{
	return AssignedDrones.IsValidIndex(SlotIndex) ? AssignedDrones[SlotIndex] : nullptr;
}

int32 ABaseBuilding::GetAssignedDroneCount() const
{
	int32 AssignedCount = 0;
	for (const ABaseDrone* Drone : AssignedDrones)
	{
		AssignedCount += IsValid(Drone) ? 1 : 0;
	}

	return AssignedCount;
}

float ABaseBuilding::GetCombinedDroneEfficiency() const
{
	const ESTPDroneWorkType WorkType = GetDroneWorkType();
	float CombinedEfficiency = 0.0f;
	for (const ABaseDrone* Drone : AssignedDrones)
	{
		if (IsValid(Drone))
		{
			CombinedEfficiency += Drone->GetWorkingRate(WorkType);
		}
	}

	return CombinedEfficiency;
}

bool ABaseBuilding::TryAssignDrone(ABaseDrone* Drone, int32 PreferredSlot)
{
	if (!IsValid(Drone) || !Drone->IsAvailableForAssignment() || UnlockedDroneSlots <= 0)
	{
		return false;
	}

	const int32 SlotCapacity = GetMaxDroneSlots();
	if (AssignedDrones.Num() != SlotCapacity)
	{
		AssignedDrones.SetNum(SlotCapacity);
	}

	int32 SlotIndex = PreferredSlot;
	if (SlotIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < UnlockedDroneSlots; ++Index)
		{
			if (!IsValid(AssignedDrones[Index]))
			{
				SlotIndex = Index;
				break;
			}
		}
	}

	if (SlotIndex < 0 || SlotIndex >= UnlockedDroneSlots || !AssignedDrones.IsValidIndex(SlotIndex)
		|| IsValid(AssignedDrones[SlotIndex]))
	{
		return false;
	}

	AssignedDrones[SlotIndex] = Drone;
	Drone->SetBuildingAssignmentInternal(this, SlotIndex);
	OnDroneAssignmentsChanged.Broadcast();
	return true;
}

bool ABaseBuilding::UnassignDrone(ABaseDrone* Drone)
{
	if (!IsValid(Drone))
	{
		return false;
	}

	const int32 SlotIndex = AssignedDrones.IndexOfByKey(Drone);
	return SlotIndex != INDEX_NONE && UnassignDroneAtSlot(SlotIndex);
}

bool ABaseBuilding::UnassignDroneAtSlot(int32 SlotIndex)
{
	if (!AssignedDrones.IsValidIndex(SlotIndex) || !IsValid(AssignedDrones[SlotIndex]))
	{
		return false;
	}

	ABaseDrone* Drone = AssignedDrones[SlotIndex];
	AssignedDrones[SlotIndex] = nullptr;
	Drone->SetBuildingAssignmentInternal(nullptr, INDEX_NONE);
	OnDroneAssignmentsChanged.Broadcast();
	return true;
}

void ABaseBuilding::SetUnlockedDroneSlots(int32 NewUnlockedSlots)
{
	const int32 ClampedSlots = FMath::Clamp(NewUnlockedSlots, 0, GetMaxDroneSlots());
	if (UnlockedDroneSlots == ClampedSlots)
	{
		return;
	}

	for (int32 SlotIndex = AssignedDrones.Num() - 1; SlotIndex >= ClampedSlots; --SlotIndex)
	{
		UnassignDroneAtSlot(SlotIndex);
	}

	UnlockedDroneSlots = ClampedSlots;
	OnDroneSlotsChanged.Broadcast(UnlockedDroneSlots);
}

void ABaseBuilding::UnlockDroneSlots(int32 SlotsToUnlock)
{
	if (SlotsToUnlock > 0)
	{
		SetUnlockedDroneSlots(UnlockedDroneSlots + SlotsToUnlock);
	}
}

void ABaseBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int32 SlotIndex = 0; SlotIndex < AssignedDrones.Num(); ++SlotIndex)
	{
		UnassignDroneAtSlot(SlotIndex);
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			It->ReleaseCells(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABaseBuilding::ConfigureMesh()
{
	BuildingMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuildingMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BuildingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BuildingMesh->SetGenerateOverlapEvents(false);
}

void ABaseBuilding::SetConstructionProgress(float NewProgress)
{
	ConstructionProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	RefreshConstructionProgressBar();
}

void ABaseBuilding::ShowConstructionProgress()
{
	if (ConstructionProgressBar)
	{
		ConstructionProgressBar->SetHiddenInGame(false);
	}
}

void ABaseBuilding::HideConstructionProgress()
{
	if (ConstructionProgressBar)
	{
		ConstructionProgressBar->SetHiddenInGame(true);
	}
}

void ABaseBuilding::SetPlacementPreview(bool bPreview)
{
	if (BuildingMesh && bPreview && !bPlacementPreview && PlacementMaterial)
	{
		PlacementOriginalOverlay = BuildingMesh->GetOverlayMaterial();
		bPlacementOriginalDisallowNanite = BuildingMesh->bDisallowNanite;
		BuildingMesh->bDisallowNanite = true;
		BuildingMesh->MarkRenderStateDirty();
		PlacementGhostMaterial = UMaterialInstanceDynamic::Create(PlacementMaterial, this);
		PlacementGhostMaterial->SetScalarParameterValue(TEXT("PlacementOpacity"), 0.16f);
		BuildingMesh->SetOverlayMaterial(PlacementGhostMaterial);
		PlacementLineMaterial = UMaterialInstanceDynamic::Create(PlacementMaterial, this);
		PlacementLineMaterial->SetScalarParameterValue(TEXT("PlacementOpacity"), 0.85f);
		for (int32 Index = 0; Index < 90; ++Index)
		{
			UStaticMeshComponent* Line = NewObject<UStaticMeshComponent>(this);
			Line->SetupAttachment(SceneRoot);
			Line->SetStaticMesh(PlacementLineMesh);
			Line->SetMaterial(0, PlacementLineMaterial);
			Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Line->SetCastShadow(false);
			Line->RegisterComponent();
			PlacementLines.Add(Line);
		}
	}
	else if (!bPreview && bPlacementPreview)
	{
		if (BuildingMesh)
		{
			BuildingMesh->bDisallowNanite = bPlacementOriginalDisallowNanite;
			BuildingMesh->MarkRenderStateDirty();
		}
		if (BuildingMesh) BuildingMesh->SetOverlayMaterial(PlacementOriginalOverlay);
		for (UStaticMeshComponent* Line : PlacementLines) if (Line) Line->DestroyComponent();
		PlacementLines.Reset();
		PlacementOriginalOverlay = nullptr;
		PlacementGhostMaterial = nullptr;
		PlacementLineMaterial = nullptr;
	}
	if (BuildingMesh && bPreview && !bPlacementPreviewMeshLocationSaved)
	{
		PlacementPreviewMeshLocation = BuildingMesh->GetRelativeLocation();
		bPlacementPreviewMeshLocationSaved = true;
		BuildingMesh->SetRelativeLocation(PlacementPreviewMeshLocation + FVector(0.0f, 0.0f, 15.0f));
	}
	else if (BuildingMesh && !bPreview && bPlacementPreviewMeshLocationSaved)
	{
		BuildingMesh->SetRelativeLocation(PlacementPreviewMeshLocation);
		bPlacementPreviewMeshLocationSaved = false;
	}

	bPlacementPreview = bPreview;
	bIsSelectable = !bPreview;
	SetActorEnableCollision(!bPreview);
	SetConstructionProgress(bPreview ? 1.0f : 0.0f);
	if (BuildingMesh)
	{
		BuildingMesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		BuildingMesh->SetRenderCustomDepth(bPreview);
		BuildingMesh->SetCustomDepthStencilValue(bPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}

void ABaseBuilding::SetPlacementPreviewValid(bool bValidPlacement)
{
	bPlacementPreviewValid = bValidPlacement;
	if (!BuildingMesh || !bPlacementPreview)
	{
		return;
	}

	BuildingMesh->SetCustomDepthStencilValue(bValidPlacement ? 2 : 3);
	const FLinearColor Color = bValidPlacement ? FLinearColor(0.08f, 0.85f, 0.42f) : FLinearColor(1, 0.16f, 0.12f);
	if (PlacementGhostMaterial) PlacementGhostMaterial->SetVectorParameterValue(TEXT("PlacementColor"), Color);
	if (PlacementLineMaterial) PlacementLineMaterial->SetVectorParameterValue(TEXT("PlacementColor"), Color);

	APlanetSurfaceManager* Surface = nullptr;
	for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
	{
		Surface = *It;
		break;
	}
	if (!Surface)
	{
		return;
	}

	const float CellSize = Surface->GetTileSpacing();
	const FIntPoint Footprint = GetGridFootprint();
	const float HalfX = Footprint.X * CellSize * 0.5f;
	const float HalfY = Footprint.Y * CellSize * 0.5f;
	const float Gap = Surface->GetBuildingClearanceCells() * CellSize;
	int32 LineIndex = 0;
	auto SetLine = [&](const FVector& Center, float Length, bool bHorizontal, float Width)
	{
		if (!PlacementLines.IsValidIndex(LineIndex)) return;
		UStaticMeshComponent* Line = PlacementLines[LineIndex++];
		Line->SetRelativeLocation(Center);
		Line->SetRelativeScale3D(bHorizontal ? FVector(Length / 100, Width / 100, 0.01f) : FVector(Width / 100, Length / 100, 0.01f));
	};
	// Fine footprint, dashed clearance perimeter, and a restrained holographic grid.
	for (int32 Edge = 0; Edge < 4; ++Edge)
	{
		const bool bHorizontal = Edge < 2;
		const float Sign = Edge % 2 == 0 ? -1.0f : 1.0f;
		SetLine(bHorizontal ? FVector(0, Sign * HalfY, 12) : FVector(Sign * HalfX, 0, 12),
			2 * (bHorizontal ? HalfX : HalfY), bHorizontal, 1.2f);
		const float Along = (bHorizontal ? HalfX : HalfY) + Gap;
		const float Across = (bHorizontal ? HalfY : HalfX) + Gap;
		for (int32 Dash = 0; Dash < 16; ++Dash)
		{
			const float Position = -Along + (Dash + 0.5f) * (2 * Along / 16);
			SetLine(bHorizontal ? FVector(Position, Sign * Across, 12) : FVector(Sign * Across, Position, 12),
				2 * Along / 16 * 0.65f, bHorizontal, 3.0f);
		}
	}
	for (int32 Grid = 1; Grid <= 11; ++Grid)
	{
		const float Fraction = Grid / 12.0f;
		SetLine(FVector(0, FMath::Lerp(-HalfY, HalfY, Fraction), 11), 2 * HalfX, true, 0.35f);
		SetLine(FVector(FMath::Lerp(-HalfX, HalfX, Fraction), 0, 11), 2 * HalfY, false, 0.35f);
	}
}

void ABaseBuilding::RefreshConstructionProgressBar()
{
	if (!ConstructionProgressBar)
	{
		return;
	}

	ConstructionProgressBar->SetHiddenInGame(ConstructionProgress >= 1.0f);

	if (UConstructionProgressBarWidget* ProgressWidget = Cast<UConstructionProgressBarWidget>(ConstructionProgressBar->GetUserWidgetObject()))
	{
		ProgressWidget->SetProgress(ConstructionProgress);
	}
}

FIntPoint ABaseBuilding::GetGridFootprint() const
{
	const UStaticMesh* Mesh = BuildingMesh ? BuildingMesh->GetStaticMesh() : nullptr;
	if (!Mesh && IsValid(BuildingData))
	{
		Mesh = BuildingData->BuildingMesh.Get();
	}
	if (!Mesh)
	{
		return FIntPoint(1, 1);
	}

	// Convert the imported mesh bounds through the component transform so BP scale,
	// rotation and pivot offsets are included in the occupied grid area.
	const FTransform MeshToActor = BuildingMesh ? BuildingMesh->GetRelativeTransform() : FTransform::Identity;
	const FVector Size = Mesh->GetBoundingBox().TransformBy(MeshToActor).GetSize().GetAbs();
	float CellSize = 100.0f;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
		{
			CellSize = FMath::Max(1.0f, It->GetTileSpacing());
			break;
		}
	}

	return FIntPoint(
		FMath::Max(1, FMath::CeilToInt((Size.X - KINDA_SMALL_NUMBER) / CellSize)),
		FMath::Max(1, FMath::CeilToInt((Size.Y - KINDA_SMALL_NUMBER) / CellSize)));
}

ESTPBuildingType ABaseBuilding::GetBuildingType() const
{
	return IsValid(BuildingData) ? BuildingData->BuildingType : BuildingType;
}

FText ABaseBuilding::GetBuildingDisplayName() const
{
	return IsValid(BuildingData) ? BuildingData->DisplayName : BuildingDisplayName;
}

FText ABaseBuilding::GetBuildingDescription() const
{
	return IsValid(BuildingData) ? BuildingData->Description : BuildingDescription;
}

UTexture2D* ABaseBuilding::GetBuildingThumbnail() const
{
	return IsValid(BuildingData) ? BuildingData->Thumbnail.Get() : BuildingThumbnail.Get();
}

int32 ABaseBuilding::GetMaxDroneSlots() const
{
	return FMath::Max(0, IsValid(BuildingData) ? BuildingData->MaxDroneSlots : MaxDroneSlots);
}

int32 ABaseBuilding::GetInitiallyUnlockedDroneSlots() const
{
	return FMath::Max(0, IsValid(BuildingData) ? BuildingData->InitiallyUnlockedDroneSlots : InitiallyUnlockedDroneSlots);
}

ESTPDroneWorkType ABaseBuilding::GetDroneWorkType() const
{
	return IsValid(BuildingData) ? BuildingData->DroneWorkType : DroneWorkType;
}

const TArray<FResourceCost>& ABaseBuilding::GetConstructionCosts() const
{
	return IsValid(BuildingData) ? BuildingData->ConstructionCosts : ConstructionCosts;
}

float ABaseBuilding::GetMaxHealth() const
{
	return FMath::Max(0.0f, IsValid(BuildingData) ? BuildingData->MaxHealth : MaxHealth);
}

FName ABaseBuilding::GetBuildingTag() const
{
	return IsValid(BuildingData) ? BuildingData->BuildingTag : BuildingTag;
}
