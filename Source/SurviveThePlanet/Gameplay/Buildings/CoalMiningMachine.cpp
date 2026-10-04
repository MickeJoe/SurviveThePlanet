#include "Gameplay/Buildings/CoalMiningMachine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACoalMiningMachine::ACoalMiningMachine()
{
	SupportedResourceTypes = {EResourceType::Coal};
	Cutter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoalCutter"));
	Cutter->SetupAttachment(BuildingMesh);
	Cutter->SetRelativeLocation(FVector(-25.0f, 61.0f, 35.0f));
	Cutter->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CutterAsset(TEXT("/Game/Units/Buildings/Coal/Meshes/SM_CoalCutter.SM_CoalCutter"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChipAsset(TEXT("/Game/Units/Buildings/Coal/Meshes/SM_CoalChip.SM_CoalChip"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DustAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DustMaterial(TEXT("/Game/Units/Buildings/Coal/Materials/M_CoalDust.M_CoalDust"));
	Cutter->SetStaticMesh(CutterAsset.Object);
	ConveyorCoal = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ConveyorCoal"));
	ConveyorCoal->SetupAttachment(BuildingMesh);
	ConveyorCoal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CuttingChips = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CuttingChips"));
	CuttingChips->SetupAttachment(BuildingMesh);
	CuttingChips->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CuttingChips->SetCastShadow(false);
	CuttingDust = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CuttingDust"));
	CuttingDust->SetupAttachment(BuildingMesh);
	CuttingDust->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CuttingDust->SetCastShadow(false);
	CuttingDust->bDisallowNanite = true;
	ConveyorCoal->SetStaticMesh(ChipAsset.Object);
	CuttingChips->SetStaticMesh(ChipAsset.Object);
	CuttingDust->SetStaticMesh(DustAsset.Object);
	CuttingDust->SetMaterial(0, DustMaterial.Object);
	for (UStaticMeshComponent* Component : {Cutter.Get(), static_cast<UStaticMeshComponent*>(ConveyorCoal.Get()),
		static_cast<UStaticMeshComponent*>(CuttingChips.Get()), static_cast<UStaticMeshComponent*>(CuttingDust.Get())})
	{
		Component->SetMobility(EComponentMobility::Movable);
	}

}

void ACoalMiningMachine::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitializeOperatingVisuals();
}

void ACoalMiningMachine::BeginPlay()
{
	Super::BeginPlay();
	// Blueprint component overrides may be empty after compilation or asset reload.
	// Bind the moving parts on the actual spawned actor, not just the editor CDO.
	InitializeOperatingVisuals();
}

void ACoalMiningMachine::InitializeOperatingVisuals()
{
	if (!Cutter->GetStaticMesh())
		Cutter->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Buildings/Coal/Meshes/SM_CoalCutter.SM_CoalCutter")));
	UStaticMesh* ChipMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Units/Buildings/Coal/Meshes/SM_CoalChip.SM_CoalChip"));
	ConveyorCoal->SetStaticMesh(ChipMesh);
	CuttingChips->SetStaticMesh(ChipMesh);
	CuttingDust->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	if (UMaterialInterface* DustMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Units/Buildings/Coal/Materials/M_CoalDust.M_CoalDust")))
		CuttingDust->SetMaterial(0, DustMaterial);
	Cutter->SetRelativeLocation(FVector(-25.0f, 61.0f, 35.0f));
	ConveyorCoal->ClearInstances();
	CuttingChips->ClearInstances();
	CuttingDust->ClearInstances();
	for (int32 Index = 0; Index < 6; ++Index) ConveyorCoal->AddInstance(FTransform::Identity);
	for (int32 Index = 0; Index < 16; ++Index) CuttingChips->AddInstance(FTransform::Identity);
	for (int32 Index = 0; Index < 8; ++Index) CuttingDust->AddInstance(FTransform::Identity);
	UpdateOperatingVisuals(false);
}

void ACoalMiningMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bOperating = IsProducingResource();
	if (bOperating) OperatingTime += DeltaSeconds * FMath::Clamp(GetCombinedDroneEfficiency(), 1.0f, 3.0f);
	UpdateOperatingVisuals(bOperating);
}

void ACoalMiningMachine::UpdateOperatingVisuals(bool bOperating)
{
	Cutter->SetRelativeRotation(FRotator(0.0f, 0.0f, FMath::Fmod(OperatingTime * 145.0f, 360.0f)));
	const bool bShowPayload = !IsPlacementPreview() && GetConstructionProgress() >= 1.0f;
	ConveyorCoal->SetVisibility(bShowPayload);
	CuttingChips->SetVisibility(bOperating);
	CuttingDust->SetVisibility(bOperating);
	for (int32 Index = 0; Index < ConveyorCoal->GetInstanceCount(); ++Index)
	{
		const float Phase = FMath::Frac(OperatingTime * 0.35f + Index / 6.0f);
		const FVector Position(48.0f + Phase * 107.0f, -20.0f, 68.0f);
		ConveyorCoal->UpdateInstanceTransform(Index, FTransform(FRotator(0, Index * 43.0f, 0), Position, FVector(2.5f)), false, false, true);
	}
	ConveyorCoal->MarkRenderStateDirty();
	for (int32 Index = 0; Index < CuttingChips->GetInstanceCount(); ++Index)
	{
		const float Phase = FMath::Frac(OperatingTime * 1.8f + Index / 16.0f);
		const FVector Position(-83.0f + Index * 7.5f, 70.0f + Phase * 48.0f, 18.0f + 52.0f * FMath::Sin(Phase * PI));
		CuttingChips->UpdateInstanceTransform(Index, FTransform(FRotator(Phase * 360, Index * 31, 0), Position, FVector(2.8f - Phase * 1.8f)), false, false, true);
	}
	CuttingChips->MarkRenderStateDirty();
	for (int32 Index = 0; Index < CuttingDust->GetInstanceCount(); ++Index)
	{
		const float Phase = FMath::Frac(OperatingTime * 0.7f + Index / 8.0f);
		const FVector Position(-75.0f + (Index % 4) * 34.0f, 77.0f + Phase * 65.0f, 18.0f + Phase * 75.0f);
		const float Scale = FMath::Sin(Phase * PI) * (0.45f + Phase * 0.35f);
		CuttingDust->UpdateInstanceTransform(Index, FTransform(FRotator::ZeroRotator, Position, FVector(Scale, Scale, Scale * 0.7f)), false, false, true);
	}
	CuttingDust->MarkRenderStateDirty();
}
