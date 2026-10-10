#include "Gameplay/Trading/MerchantShip.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Gameplay/Buildings/CargoBay.h"
#include "Gameplay/Trading/MerchantDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float LandingSeconds = 9.0f;
	constexpr float DepartureSeconds = 7.0f;
	float Smooth(float Value) { return Value * Value * (3.0f - 2.0f * Value); }
}

AMerchantShip::AMerchantShip()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
	SetActorEnableCollision(false);
}

UStaticMeshComponent* AMerchantShip::AddVisual(FName Name, UStaticMesh* Mesh)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
	Component->SetupAttachment(RootComponent);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

void AMerchantShip::Arrive(UMerchantDefinition* Definition, ACargoBay* CargoBay)
{
	check(Definition && CargoBay);
	Merchant = Definition;
	DockTransform = CargoBay->GetMerchantDockTransform();
	SetActorScale3D(FVector(Definition->ShipScale));
	Hull = AddVisual(TEXT("Hull"), Definition->ShipMesh);
	LandingGear = AddVisual(TEXT("LandingGear"), Definition->LandingGearMesh);
	LoadingRamp = AddVisual(TEXT("LoadingRamp"), Definition->LoadingRampMesh);
	LoadingRamp->SetRelativeLocation(Definition->LoadingRampPivot);
	const FString FXPath = TEXT("/Game/Units/Trading/Ships/Shared/");
	UStaticMesh* ExhaustMesh = LoadObject<UStaticMesh>(nullptr, *(FXPath + TEXT("SM_MerchantExhaust.SM_MerchantExhaust")));
	UStaticMesh* DustMesh = LoadObject<UStaticMesh>(nullptr, *(FXPath + TEXT("SM_MerchantDust.SM_MerchantDust")));
	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, *(FXPath + TEXT("M_MerchantExhaust.M_MerchantExhaust")));
	UMaterialInterface* Dust = LoadObject<UMaterialInterface>(nullptr, *(FXPath + TEXT("M_MerchantDust.M_MerchantDust")));
	if (Glow) ExhaustMaterial = UMaterialInstanceDynamic::Create(Glow, this);
	for (int32 Index = 0; Index < Definition->ThrusterLocations.Num(); ++Index)
	{
		UStaticMeshComponent* Exhaust = AddVisual(*FString::Printf(TEXT("Exhaust%d"), Index), ExhaustMesh);
		Exhaust->SetRelativeLocation(Definition->ThrusterLocations[Index]);
		Exhaust->SetCastShadow(false);
		if (ExhaustMaterial) Exhaust->SetMaterial(0, ExhaustMaterial);
		Exhausts.Add(Exhaust);
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UStaticMeshComponent* Cloud = AddVisual(*FString::Printf(TEXT("LandingDust%d"), Index), DustMesh);
		Cloud->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Cloud->SetCastShadow(false);
		if (Dust)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Dust, this);
			Cloud->SetMaterial(0, Material);
			DustMaterials.Add(Material);
		}
		DustClouds.Add(Cloud);
	}
	Phase = EFlightPhase::Arriving;
	UpdateFlight(0.0f);
}

void AMerchantShip::Depart()
{
	if (Phase == EFlightPhase::Departing) return;
	Phase = EFlightPhase::Departing;
	FlightElapsed = 0.0f;
	DepartureStart = GetActorLocation();
	DepartureRotation = GetActorRotation();
}

void AMerchantShip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dilation = UGameplayStatics::GetGlobalTimeDilation(this);
	if (Dilation < 0.001f) return;
	// Actor delta already includes the selected simulation speed.
	VisualElapsed += DeltaSeconds;
	UpdateFlight(DeltaSeconds);
}

void AMerchantShip::UpdateFlight(float DeltaSeconds)
{
	if (!Merchant) return;
	FlightElapsed += DeltaSeconds;
	float Thrust = 0.0f;
	float NearDeck = 0.0f;
	FRotator Rotation = DockTransform.Rotator();
	float GearExtension = 1.0f;
	float RampOpening = 0.0f;
	if (Phase == EFlightPhase::Arriving)
	{
		const float Progress = FMath::Clamp(FlightElapsed / LandingSeconds, 0.0f, 1.0f);
		FVector Offset;
		if (Progress < 0.65f)
		{
			const float Approach = Smooth(Progress / 0.65f);
			Offset = FMath::Lerp(FVector(-2600, 650, 1500), FVector(0, 0, 210), Approach);
			Rotation.Pitch += 7.0f * (1.0f - Approach);
		}
		else Offset = FVector(0, 0, 210.0f * (1.0f - Smooth((Progress - 0.65f) / 0.35f)));
		SetActorLocationAndRotation(DockTransform.TransformPositionNoScale(Offset), Rotation);
		GearExtension = FMath::Lerp(0.25f, 1.0f, Smooth(FMath::Clamp((Progress - .40f) / .35f, 0.0f, 1.0f)));
		Thrust = Progress < .90f ? 1.0f : (1.0f - Progress) * 10.0f;
		NearDeck = FMath::Clamp(1.0f - Offset.Z / 260.0f, 0.0f, 1.0f) * Thrust;
		if (Progress >= 1.0f) { Phase = EFlightPhase::Docked; FlightElapsed = 0.0f; }
	}
	else if (Phase == EFlightPhase::Docked)
	{
		SetActorLocationAndRotation(DockTransform.GetLocation(), Rotation);
		RampOpening = Smooth(FMath::Clamp(FlightElapsed / 1.0f, 0.0f, 1.0f));
	}
	else
	{
		const float Progress = FMath::Clamp(FlightElapsed / DepartureSeconds, 0.0f, 1.0f);
		// Shut the cargo hatch first, rise vertically, then accelerate out of view.
		RampOpening = 1.0f - Smooth(FMath::Clamp(FlightElapsed / .8f, 0.0f, 1.0f));
		FVector Offset;
		if (Progress < .40f) Offset = FVector(0, 0, 300.0f * Smooth(Progress / .40f));
		else Offset = FMath::Lerp(FVector(0, 0, 300), FVector(3200, -700, 1700), Smooth((Progress - .40f) / .60f));
		Rotation.Pitch -= 8.0f * FMath::Clamp((Progress - .35f) / .30f, 0.0f, 1.0f);
		SetActorLocationAndRotation(DepartureStart + DockTransform.TransformVectorNoScale(Offset), Rotation);
		GearExtension = FMath::Lerp(1.0f, .25f, Smooth(FMath::Clamp((Progress - .25f) / .30f, 0.0f, 1.0f)));
		Thrust = FMath::Clamp(FlightElapsed / .6f, 0.0f, 1.0f);
		NearDeck = FMath::Clamp(1.0f - Offset.Z / 260.0f, 0.0f, 1.0f) * Thrust;
		if (Progress >= 1.0f) { Destroy(); return; }
	}
	LandingGear->SetRelativeScale3D(FVector(1, 1, GearExtension));
	LoadingRamp->SetRelativeRotation(FRotator(65.0f * RampOpening, 0, 0));
	UpdateEffects(Thrust, NearDeck);
}

void AMerchantShip::UpdateEffects(float Thrust, float NearDeck)
{
	const float Flicker = 1.0f + 0.055f * FMath::Sin(VisualElapsed * 27.0f);
	if (ExhaustMaterial) ExhaustMaterial->SetScalarParameterValue(TEXT("Thrust"), Thrust * Flicker);
	for (UStaticMeshComponent* Exhaust : Exhausts)
	{
		Exhaust->SetVisibility(Thrust > 0.01f);
		Exhaust->SetRelativeScale3D(FVector(.7f, .7f, .55f + Thrust * .8f * Flicker));
	}
	for (int32 Index = 0; Index < DustClouds.Num(); ++Index)
	{
		const float Age = FMath::Fmod(VisualElapsed * .7f + Index / 3.0f, 1.0f);
		const float Radius = Merchant->ShipScale * (1.0f + Age * 2.8f);
		DustClouds[Index]->SetWorldLocation(DockTransform.GetLocation() + FVector(0, 0, 5.0f + Index * 3.0f));
		DustClouds[Index]->SetWorldRotation(FRotator(0, Index * 53.0f + VisualElapsed * 11.0f, 0));
		DustClouds[Index]->SetWorldScale3D(FVector(Radius, Radius, 1.0f));
		DustClouds[Index]->SetVisibility(NearDeck > .01f);
		if (DustMaterials.IsValidIndex(Index)) DustMaterials[Index]->SetScalarParameterValue(TEXT("Strength"), NearDeck * (1.0f - Age) * .35f);
	}
}
