#include "Gameplay/Base/BaseBuilding.h"

#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
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
}

void ABaseBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (BuildingMesh)
	{
		BuildingMesh->SetStaticMesh(BaseModuleMesh);
	}
}

void ABaseBuilding::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (!BuildingTag.IsNone())
	{
		Tags.AddUnique(BuildingTag);
	}

	RefreshConstructionProgressBar();
}

void ABaseBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
