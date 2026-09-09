#include "Gameplay/Buildings/Steelworks.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace
{
	constexpr float DefaultIronPerCycle = 4.0f;
	constexpr float DefaultSteelPerCycle = 2.0f;
	constexpr float DefaultCycleSeconds = 20.0f;
	const FVector SlabStart(105.0f, -72.0f, 72.0f);
	const FVector SlabEnd(270.0f, -72.0f, 72.0f);
}

ASteelworks::ASteelworks()
{
	BuildingTag = TEXT("Steelworks");
	BuildingType = ESTPBuildingType::Steelworks;
	BuildingDisplayName = NSLOCTEXT("SurviveThePlanet", "DefaultSteelworksName", "Steelworks");
	BuildingDescription = NSLOCTEXT("SurviveThePlanet", "DefaultSteelworksDescription", "Consumes iron and electricity to produce steel.");
	EnergyConsumptionPerMinute = 18.0f;
	ConstructionProgress = 0.0f;

	auto ConfigureMeshComponent = [this](const TCHAR* Name, const FVector& Location)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(SceneRoot);
		Component->SetRelativeLocation(Location);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		return Component;
	};

	ProcessDrumA = ConfigureMeshComponent(TEXT("ProcessDrumA"), FVector(52.0f, 112.0f, 145.0f));
	ProcessDrumB = ConfigureMeshComponent(TEXT("ProcessDrumB"), FVector(105.0f, -124.0f, 126.0f));
	FurnaceGlowMesh = ConfigureMeshComponent(TEXT("FurnaceGlowMesh"), FVector(65.0f, -72.0f, 91.0f));
	SteelSlabA = ConfigureMeshComponent(TEXT("SteelSlabA"), SlabStart);
	SteelSlabB = ConfigureMeshComponent(TEXT("SteelSlabB"), SlabStart);
	SteelSlabC = ConfigureMeshComponent(TEXT("SteelSlabC"), SlabStart);

	FurnaceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FurnaceLight"));
	FurnaceLight->SetupAttachment(SceneRoot);
	FurnaceLight->SetRelativeLocation(FVector(85.0f, -72.0f, 98.0f));
	FurnaceLight->SetLightColor(FLinearColor(1.0f, 0.22f, 0.015f));
	FurnaceLight->SetIntensityUnits(ELightUnits::Candelas);
	FurnaceLight->SetIntensity(0.0f);
	FurnaceLight->SetAttenuationRadius(280.0f);
	FurnaceLight->SetCastShadows(false);

	auto ConfigureFX = [this](const TCHAR* Name, const FVector& Location)
	{
		UNiagaraComponent* Component = CreateDefaultSubobject<UNiagaraComponent>(Name);
		Component->SetupAttachment(SceneRoot);
		Component->SetRelativeLocation(Location);
		Component->SetAutoActivate(false);
		return Component;
	};
	SparkFX = ConfigureFX(TEXT("SparkFX"), FVector(98.0f, -72.0f, 87.0f));
	SteamFX = ConfigureFX(TEXT("SteamFX"), FVector(-72.0f, -133.0f, 132.0f));
	SmokeFX = ConfigureFX(TEXT("SmokeFX"), FVector(-48.0f, 78.0f, 263.0f));
}

void ASteelworks::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualAssets();
	RefreshPreviewVisuals();
}

void ASteelworks::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisualAssets();
	CachedResourceManager = ResolveResourceManager();
	SetProductionActive(false);
}

void ASteelworks::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AResourceManager* Manager = ResolveResourceManager();
	const USteelworksBuildingDataAsset* Data = GetSteelworksData();
	const float CycleSeconds = Data ? FMath::Max(0.01f, Data->CycleSeconds) : DefaultCycleSeconds;
	const int32 IronPerCycle = FMath::CeilToInt(Data ? Data->IronPerCycle : DefaultIronPerCycle);
	const int32 SteelPerCycle = FMath::FloorToInt(Data ? Data->SteelPerCycle : DefaultSteelPerCycle);
	const bool bCanProduce = !bPlacementPreview
		&& GetConstructionProgress() >= 1.0f
		&& IsOperational()
		&& Manager
		&& SteelPerCycle > 0
		&& (IronPerCycle <= 0 || Manager->HasResource(EResourceType::Iron, IronPerCycle));

	SetProductionActive(bCanProduce);
	if (!bIsProducing)
	{
		return;
	}

	UpdateActiveVisuals(FMath::Max(0.0f, DeltaSeconds));
	CycleProgressSeconds += FMath::Max(0.0f, DeltaSeconds);
	while (CycleProgressSeconds >= CycleSeconds)
	{
		if (IronPerCycle > 0 && !Manager->TrySpendResource(EResourceType::Iron, IronPerCycle))
		{
			CycleProgressSeconds = FMath::Min(CycleProgressSeconds, CycleSeconds);
			SetProductionActive(false);
			return;
		}

		Manager->AddResource(EResourceType::Steel, SteelPerCycle);
		CycleProgressSeconds -= CycleSeconds;
	}
}

float ASteelworks::GetSteelProductionPerMinute() const
{
	const USteelworksBuildingDataAsset* Data = GetSteelworksData();
	const float Output = Data ? Data->SteelPerCycle : DefaultSteelPerCycle;
	const float CycleSeconds = Data ? Data->CycleSeconds : DefaultCycleSeconds;
	return FMath::Max(0.0f, Output) * 60.0f / FMath::Max(0.01f, CycleSeconds);
}

float ASteelworks::GetIronConsumptionPerMinute() const
{
	const USteelworksBuildingDataAsset* Data = GetSteelworksData();
	const float Input = Data ? Data->IronPerCycle : DefaultIronPerCycle;
	const float CycleSeconds = Data ? Data->CycleSeconds : DefaultCycleSeconds;
	return FMath::Max(0.0f, Input) * 60.0f / FMath::Max(0.01f, CycleSeconds);
}

void ASteelworks::SetPlacementPreview(bool bPreview)
{
	Super::SetPlacementPreview(bPreview);
	if (bPreview)
	{
		SetProductionActive(false);
	}
	RefreshPreviewVisuals();
}

void ASteelworks::SetPlacementPreviewValid(bool bValidPlacement)
{
	Super::SetPlacementPreviewValid(bValidPlacement);
	RefreshPreviewVisuals();
}

const USteelworksBuildingDataAsset* ASteelworks::GetSteelworksData() const
{
	return Cast<USteelworksBuildingDataAsset>(GetBuildingData());
}

AResourceManager* ASteelworks::ResolveResourceManager()
{
	if (IsValid(CachedResourceManager))
	{
		return CachedResourceManager;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AResourceManager> It(World); It; ++It)
		{
			CachedResourceManager = *It;
			break;
		}
	}
	return CachedResourceManager;
}

void ASteelworks::ApplyVisualAssets()
{
	if (ProcessDrumA) ProcessDrumA->SetStaticMesh(ProcessDrumMeshAsset);
	if (ProcessDrumB) ProcessDrumB->SetStaticMesh(ProcessDrumMeshAsset);
	if (FurnaceGlowMesh) FurnaceGlowMesh->SetStaticMesh(FurnaceGlowMeshAsset);
	if (SteelSlabA) SteelSlabA->SetStaticMesh(SteelSlabMeshAsset);
	if (SteelSlabB) SteelSlabB->SetStaticMesh(SteelSlabMeshAsset);
	if (SteelSlabC) SteelSlabC->SetStaticMesh(SteelSlabMeshAsset);
	if (SparkFX) SparkFX->SetAsset(SparkSystem);
	if (SteamFX) SteamFX->SetAsset(SteamSystem);
	if (SmokeFX) SmokeFX->SetAsset(SmokeSystem);
}

void ASteelworks::SetProductionActive(bool bNewActive)
{
	if (bVisualStateInitialized && bIsProducing == bNewActive)
	{
		return;
	}

	bVisualStateInitialized = true;
	bIsProducing = bNewActive;
	for (UStaticMeshComponent* Slab : {SteelSlabA.Get(), SteelSlabB.Get(), SteelSlabC.Get()})
	{
		if (Slab) Slab->SetVisibility(bIsProducing, true);
	}
	if (FurnaceGlowMesh) FurnaceGlowMesh->SetVisibility(bIsProducing, true);
	if (FurnaceLight) FurnaceLight->SetVisibility(bIsProducing, true);

	for (UNiagaraComponent* FX : {SteamFX.Get(), SmokeFX.Get()})
	{
		if (!FX) continue;
		if (bIsProducing && FX->GetAsset()) FX->Activate(true);
		else FX->DeactivateImmediate();
	}
	if (SparkFX)
	{
		SparkFX->DeactivateImmediate();
	}

	if (!bIsProducing && FurnaceLight)
	{
		FurnaceLight->SetIntensity(0.0f);
	}
	OnProductionStateChanged.Broadcast(bIsProducing);
}

void ASteelworks::UpdateActiveVisuals(float DeltaSeconds)
{
	VisualTimeSeconds += DeltaSeconds;
	SparkAccumulatorSeconds += DeltaSeconds;

	const float RotationStep = DrumRotationDegreesPerSecond * DeltaSeconds;
	if (ProcessDrumA) ProcessDrumA->AddLocalRotation(FRotator(0.0f, RotationStep, 0.0f));
	if (ProcessDrumB) ProcessDrumB->AddLocalRotation(FRotator(0.0f, -RotationStep * 0.82f, 0.0f));

	const float LoopSeconds = FMath::Max(0.1f, VisualLoopSeconds);
	UStaticMeshComponent* Slabs[] = {SteelSlabA.Get(), SteelSlabB.Get(), SteelSlabC.Get()};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slabs); ++Index)
	{
		if (!Slabs[Index]) continue;
		const float Alpha = FMath::Fmod(VisualTimeSeconds / LoopSeconds + Index / 3.0f, 1.0f);
		Slabs[Index]->SetRelativeLocation(FMath::Lerp(SlabStart, SlabEnd, Alpha));
		const FVector SlabEmission = FVector(1.0f, 0.12f, 0.01f) * FMath::Lerp(8.0f, 0.25f, Alpha);
		Slabs[Index]->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), SlabEmission);
	}

	const float Pulse = 0.88f + 0.12f * FMath::Sin(VisualTimeSeconds * 5.2f);
	if (FurnaceLight) FurnaceLight->SetIntensity(1500.0f * Pulse);
	if (FurnaceGlowMesh)
	{
		FurnaceGlowMesh->SetVectorParameterValueOnMaterials(TEXT("EmissiveColor"), FVector(1.0f, 0.07f, 0.005f) * (11.0f * Pulse));
	}

	if (SparkAccumulatorSeconds >= LoopSeconds)
	{
		SparkAccumulatorSeconds = 0.0f;
		if (SparkFX && SparkFX->GetAsset()) SparkFX->Activate(true);
	}
}

void ASteelworks::RefreshPreviewVisuals()
{
	for (UStaticMeshComponent* Component : {ProcessDrumA.Get(), ProcessDrumB.Get(), FurnaceGlowMesh.Get(), SteelSlabA.Get(), SteelSlabB.Get(), SteelSlabC.Get()})
	{
		if (!Component) continue;
		Component->SetRenderCustomDepth(bPlacementPreview);
		Component->SetCustomDepthStencilValue(bPlacementPreview ? (bPlacementPreviewValid ? 2 : 3) : 0);
	}
}
