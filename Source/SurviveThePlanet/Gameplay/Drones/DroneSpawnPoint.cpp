#include "Gameplay/Drones/DroneSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Drones/ConstructionDrone.h"

DEFINE_LOG_CATEGORY_STATIC(LogDroneSpawnPoint, Log, All);

ADroneSpawnPoint::ADroneSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetupAttachment(SceneRoot);
	SpawnDirection->ArrowSize = 1.25f;
	SpawnDirection->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(SceneRoot);
	SpawnArea->SetBoxExtent(FVector(150.0f, 150.0f, 75.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnArea->SetHiddenInGame(true);

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetHiddenInGame(true);
	PreviewMeshComponent->bIsEditorOnly = true;
}

void ADroneSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PreviewMeshComponent)
	{
		UStaticMesh* EffectivePreviewMesh = GetEffectivePreviewMesh();
		PreviewMeshComponent->SetStaticMesh(EffectivePreviewMesh);
		PreviewMeshComponent->SetVisibility(EffectivePreviewMesh != nullptr);
	}

	const float AreaRadius = FMath::Max(SpawnRadius, 25.0f);
	if (SpawnArea)
	{
		SpawnArea->SetBoxExtent(FVector(AreaRadius, AreaRadius, 75.0f));
	}
}

void ADroneSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		UStaticMesh* EffectivePreviewMesh = GetEffectivePreviewMesh();
		UE_LOG(LogDroneSpawnPoint, Log, TEXT("%s spawning %d drone(s). ActiveDifficulty=%s DroneClass=%s EffectiveClass=%s SpawnOptions=%d PreviewMesh=%s"),
			*GetName(),
			GetEffectiveSpawnCount(),
			*StaticEnum<ESpawnDifficulty>()->GetNameStringByValue(static_cast<int64>(ActiveDifficulty)),
			*GetNameSafe(DroneClass),
			*GetNameSafe(GetEffectiveDroneClass()),
			SpawnOptions.Num(),
			*GetNameSafe(EffectivePreviewMesh));

		SpawnDrones();
	}
}

void ADroneSpawnPoint::SpawnDrones()
{
	if (bReplaceExistingSpawns)
	{
		ClearSpawnedDrones();
	}

	const int32 EffectiveSpawnCount = GetEffectiveSpawnCount();
	for (int32 SpawnIndex = 0; SpawnIndex < EffectiveSpawnCount; ++SpawnIndex)
	{
		SpawnSingleDrone(SpawnIndex);
	}
}

void ADroneSpawnPoint::ClearSpawnedDrones()
{
	for (AActor* Drone : SpawnedDrones)
	{
		if (IsValid(Drone))
		{
			Drone->Destroy();
		}
	}

	SpawnedDrones.Reset();
}

FTransform ADroneSpawnPoint::GetSpawnTransform(int32 SpawnIndex) const
{
	FTransform SpawnTransform = GetActorTransform();

	const int32 EffectiveSpawnCount = GetEffectiveSpawnCount();
	if (EffectiveSpawnCount > 1 && SpawnRadius > 0.0f)
	{
		const float AngleRadians = (2.0f * PI * SpawnIndex) / EffectiveSpawnCount;
		const FVector LocalOffset(
			FMath::Cos(AngleRadians) * SpawnRadius,
			FMath::Sin(AngleRadians) * SpawnRadius,
			0.0f);

		SpawnTransform.AddToTranslation(GetActorTransform().TransformVectorNoScale(LocalOffset));
	}

	return SpawnTransform;
}

AActor* ADroneSpawnPoint::SpawnSingleDrone(int32 SpawnIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogDroneSpawnPoint, Warning, TEXT("%s cannot spawn drone %d because World is null."), *GetName(), SpawnIndex);
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = SpawnCollisionHandling;

	FTransform SpawnTransform = GetSpawnTransform(SpawnIndex);
	if (APlanetSurfaceManager* SurfaceManager = FindPlanetSurfaceManager())
	{
		const FIntPoint Footprint = GetEffectiveDroneFootprint();
		const FSTPGridPlacement Placement = SurfaceManager->GetPlacementForWorldLocation(SpawnTransform.GetLocation(), Footprint);
		if (!Placement.bValid)
		{
			UE_LOG(LogDroneSpawnPoint, Warning, TEXT("%s cannot spawn drone %d because grid placement is invalid or occupied. OriginCell=(%d,%d) Footprint=(%d,%d)"),
				*GetName(),
				SpawnIndex,
				Placement.OriginCell.X,
				Placement.OriginCell.Y,
				Footprint.X,
				Footprint.Y);
			return nullptr;
		}

		SpawnTransform.SetLocation(Placement.WorldLocation);
		SpawnTransform.SetRotation(Placement.WorldRotation.Quaternion());
	}

	AActor* NewDrone = nullptr;
	UClass* EffectiveDroneClass = GetEffectiveDroneClass();
	if (EffectiveDroneClass)
	{
		NewDrone = World->SpawnActor<AActor>(EffectiveDroneClass, SpawnTransform, SpawnParameters);
	}
	else
	{
		UE_LOG(LogDroneSpawnPoint, Warning, TEXT("%s cannot spawn drone %d because no drone class is available."),
			*GetName(),
			SpawnIndex);
	}

	if (NewDrone)
	{
		if (!DroneTag.IsNone())
		{
			NewDrone->Tags.AddUnique(DroneTag);
		}

		const FName DroneTypeTag = GetDroneTypeTag();
		if (!DroneTypeTag.IsNone())
		{
			NewDrone->Tags.AddUnique(DroneTypeTag);
		}

		SpawnedDrones.Add(NewDrone);

		UE_LOG(LogDroneSpawnPoint, Log, TEXT("%s spawned drone %s at %s."),
			*GetName(),
			*GetNameSafe(NewDrone),
			*SpawnTransform.GetLocation().ToString());
	}

	return NewDrone;
}

const FDroneSpawnOption* ADroneSpawnPoint::GetBestSpawnOption() const
{
	const FDroneSpawnOption* BestOption = nullptr;
	uint8 BestDifficultyValue = 0;
	const uint8 ActiveDifficultyValue = static_cast<uint8>(ActiveDifficulty);

	for (const FDroneSpawnOption& Option : SpawnOptions)
	{
		const uint8 OptionDifficultyValue = static_cast<uint8>(Option.MinDifficulty);
		if (!Option.DroneClass || OptionDifficultyValue > ActiveDifficultyValue)
		{
			continue;
		}

		if (!BestOption || OptionDifficultyValue >= BestDifficultyValue)
		{
			BestOption = &Option;
			BestDifficultyValue = OptionDifficultyValue;
		}
	}

	return BestOption;
}

UClass* ADroneSpawnPoint::GetEffectiveDroneClass() const
{
	if (const FDroneSpawnOption* BestOption = GetBestSpawnOption())
	{
		return BestOption->DroneClass.Get();
	}

	return DroneClass ? DroneClass.Get() : AConstructionDrone::StaticClass();
}

int32 ADroneSpawnPoint::GetEffectiveSpawnCount() const
{
	if (const FDroneSpawnOption* BestOption = GetBestSpawnOption())
	{
		return FMath::Max(1, BestOption->SpawnCount);
	}

	return FMath::Max(1, SpawnCount);
}

UStaticMesh* ADroneSpawnPoint::GetEffectivePreviewMesh() const
{
	if (DronePreviewMesh)
	{
		return DronePreviewMesh;
	}

	return LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Models/Units/WorkingDrone/ConstructionDrone.ConstructionDrone"));
}

FName ADroneSpawnPoint::GetDroneTypeTag() const
{
	if (DroneType == EDroneType::Custom)
	{
		return CustomDroneType;
	}

	const UEnum* DroneTypeEnum = StaticEnum<EDroneType>();
	return DroneTypeEnum ? FName(DroneTypeEnum->GetNameStringByValue(static_cast<int64>(DroneType))) : NAME_None;
}

APlanetSurfaceManager* ADroneSpawnPoint::FindPlanetSurfaceManager() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

FIntPoint ADroneSpawnPoint::GetEffectiveDroneFootprint() const
{
	if (const UClass* EffectiveClass = GetEffectiveDroneClass())
	{
		if (const AConstructionDrone* DefaultDrone = Cast<AConstructionDrone>(EffectiveClass->GetDefaultObject()))
		{
			return DefaultDrone->GetGridFootprint();
		}
	}

	return FIntPoint(1, 1);
}