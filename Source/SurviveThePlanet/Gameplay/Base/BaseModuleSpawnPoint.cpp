#include "Gameplay/Base/BaseModuleSpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaseModuleSpawnPoint, Log, All);

ABaseModuleSpawnPoint::ABaseModuleSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetupAttachment(SceneRoot);
	SpawnDirection->ArrowSize = 1.5f;
	SpawnDirection->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));

	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SpawnBounds->SetupAttachment(SceneRoot);
	SpawnBounds->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->SetHiddenInGame(true);

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetHiddenInGame(true);
	PreviewMeshComponent->bIsEditorOnly = true;
}

void ABaseModuleSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!PreviewMeshComponent)
	{
		return;
	}

	UStaticMesh* EffectivePreviewMesh = GetEffectivePreviewMesh();
	PreviewMeshComponent->SetStaticMesh(EffectivePreviewMesh);
	PreviewMeshComponent->SetVisibility(EffectivePreviewMesh != nullptr);
}

void ABaseModuleSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnOnBeginPlay)
	{
		UStaticMesh* EffectivePreviewMesh = GetEffectivePreviewMesh();
		UE_LOG(LogBaseModuleSpawnPoint, Log, TEXT("%s spawning base module. ActiveDifficulty=%s BaseModuleClass=%s EffectiveClass=%s SpawnOptions=%d PreviewMesh=%s"),
			*GetName(),
			*StaticEnum<ESpawnDifficulty>()->GetNameStringByValue(static_cast<int64>(ActiveDifficulty)),
			*GetNameSafe(BaseModuleClass),
			*GetNameSafe(GetEffectiveBaseModuleClass()),
			SpawnOptions.Num(),
			*GetNameSafe(EffectivePreviewMesh));

		SpawnBaseModule();
	}
}

AActor* ABaseModuleSpawnPoint::SpawnBaseModule()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogBaseModuleSpawnPoint, Warning, TEXT("%s cannot spawn base module because World is null."), *GetName());
		return nullptr;
	}

	if (bReplaceExistingSpawn)
	{
		ClearSpawnedBaseModule();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = SpawnCollisionHandling;

	FTransform SpawnTransform = GetSpawnTransform();
	APlanetSurfaceManager* SurfaceManager = FindPlanetSurfaceManager();
	FSTPGridPlacement Placement;
	if (SurfaceManager)
	{
		const FIntPoint Footprint = GetEffectiveBaseModuleFootprint();
		Placement = SurfaceManager->GetPlacementForWorldLocation(SpawnTransform.GetLocation(), Footprint);
		if (!Placement.bValid)
		{
			UE_LOG(LogBaseModuleSpawnPoint, Warning, TEXT("%s cannot spawn base module because grid placement is invalid or occupied. OriginCell=(%d,%d) Footprint=(%d,%d)"),
				*GetName(),
				Placement.OriginCell.X,
				Placement.OriginCell.Y,
				Footprint.X,
				Footprint.Y);
			return nullptr;
		}

		SpawnTransform.SetLocation(Placement.WorldLocation);
		SpawnTransform.SetRotation(Placement.WorldRotation.Quaternion());
	}

	AActor* NewBaseModule = nullptr;
	UClass* EffectiveBaseModuleClass = GetEffectiveBaseModuleClass();
	if (EffectiveBaseModuleClass)
	{
		NewBaseModule = World->SpawnActor<AActor>(EffectiveBaseModuleClass, SpawnTransform, SpawnParameters);
	}
	else
	{
		UE_LOG(LogBaseModuleSpawnPoint, Warning, TEXT("%s cannot spawn base module because no base module class is available."),
			*GetName());
	}

	if (NewBaseModule)
	{
		if (!SpawnedActorTag.IsNone())
		{
			NewBaseModule->Tags.AddUnique(SpawnedActorTag);
		}

		SpawnedBaseModule = NewBaseModule;
		if (SurfaceManager && Placement.bValid)
		{
			if (ABaseBuilding* BaseBuilding = Cast<ABaseBuilding>(NewBaseModule))
			{
				if (!SurfaceManager->ReserveCells(BaseBuilding, Placement.OriginCell, BaseBuilding->GetGridFootprint()))
				{
					UE_LOG(LogBaseModuleSpawnPoint, Warning, TEXT("%s spawned base module but grid reservation failed; destroying %s."), *GetName(), *GetNameSafe(NewBaseModule));
					NewBaseModule->Destroy();
					SpawnedBaseModule = nullptr;
					return nullptr;
				}
			}
		}

		UE_LOG(LogBaseModuleSpawnPoint, Log, TEXT("%s spawned base module %s at %s."),
			*GetName(),
			*GetNameSafe(NewBaseModule),
			*SpawnTransform.GetLocation().ToString());
	}

	return SpawnedBaseModule;
}

void ABaseModuleSpawnPoint::ClearSpawnedBaseModule()
{
	if (IsValid(SpawnedBaseModule))
	{
		SpawnedBaseModule->Destroy();
	}

	SpawnedBaseModule = nullptr;
}

FTransform ABaseModuleSpawnPoint::GetSpawnTransform() const
{
	return GetActorTransform();
}

const FBaseModuleSpawnOption* ABaseModuleSpawnPoint::GetBestSpawnOption() const
{
	const FBaseModuleSpawnOption* BestOption = nullptr;
	uint8 BestDifficultyValue = 0;
	const uint8 ActiveDifficultyValue = static_cast<uint8>(ActiveDifficulty);

	for (const FBaseModuleSpawnOption& Option : SpawnOptions)
	{
		const uint8 OptionDifficultyValue = static_cast<uint8>(Option.MinDifficulty);
		if (!Option.BaseModuleClass || OptionDifficultyValue > ActiveDifficultyValue)
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

UClass* ABaseModuleSpawnPoint::GetEffectiveBaseModuleClass() const
{
	if (const FBaseModuleSpawnOption* BestOption = GetBestSpawnOption())
	{
		return BestOption->BaseModuleClass.Get();
	}

	return BaseModuleClass ? BaseModuleClass.Get() : ABaseBuilding::StaticClass();
}

UStaticMesh* ABaseModuleSpawnPoint::GetEffectivePreviewMesh() const
{
	return BaseModulePreviewMesh;
}

APlanetSurfaceManager* ABaseModuleSpawnPoint::FindPlanetSurfaceManager() const
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

FIntPoint ABaseModuleSpawnPoint::GetEffectiveBaseModuleFootprint() const
{
	if (const UClass* EffectiveClass = GetEffectiveBaseModuleClass())
	{
		if (const ABaseBuilding* DefaultBuilding = Cast<ABaseBuilding>(EffectiveClass->GetDefaultObject()))
		{
			return DefaultBuilding->GetGridFootprint();
		}
	}

	return FIntPoint(2, 2);
}
