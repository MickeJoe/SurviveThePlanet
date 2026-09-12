// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurviveThePlanetCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "SurviveThePlanet.h"

ASurviveThePlanetCameraPawn::ASurviveThePlanetCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	// Medium isometric gameplay view: enough overview for planning while keeping
	// buildings readable and showing more of their vertical silhouettes.
	CameraBoom->TargetArmLength = 2400.0f;
	CameraBoom->SetRelativeRotation(FRotator(-30.0f, 45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("IsometricCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->SetFieldOfView(60.0f);

	FogOfWarMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Materials/Exploration/M_FogOfWar_PostProcess.M_FogOfWar_PostProcess"));
}

void ASurviveThePlanetCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(FogOfWarMaterial) && CameraComponent)
	{
		FogOfWarMaterialInstance = UMaterialInstanceDynamic::Create(FogOfWarMaterial, this);
		if (FogOfWarMaterialInstance)
		{
			CameraComponent->PostProcessSettings.AddBlendable(FogOfWarMaterialInstance, 1.0f);
			CameraComponent->PostProcessBlendWeight = 1.0f;
		}
	}
	RefreshFogOfWarVisual();
}

void ASurviveThePlanetCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Navigation is player/UI interaction, not simulation. Undo global time
	// dilation so camera speed remains constant while paused and at x1-x3.
	const float GlobalDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	const float NavigationDeltaSeconds = GlobalDilation > UE_SMALL_NUMBER
		? DeltaSeconds / GlobalDilation
		: DeltaSeconds;

	PanCamera(KeyboardPanInput + GetEdgeScrollInput(), NavigationDeltaSeconds);
	RotateCamera(KeyboardRotationInput, NavigationDeltaSeconds);
	RefreshFogOfWarVisual();
}

void ASurviveThePlanetCameraPawn::RefreshFogOfWarVisual()
{
	if (!FogOfWarMaterialInstance)
	{
		return;
	}

	AHexSectorGrid* SectorGrid = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AHexSectorGrid> It(World); It; ++It)
		{
			SectorGrid = *It;
			break;
		}
	}

	if (!SectorGrid)
	{
		return;
	}

	RefreshSectorFogMask(SectorGrid);
}

void ASurviveThePlanetCameraPawn::RefreshSectorFogMask(AHexSectorGrid* SectorGrid)
{
	if (!FogOfWarMaterialInstance || !SectorGrid || SectorGrid->Sectors.IsEmpty())
	{
		return;
	}

	uint32 NewHash = GetTypeHash(SectorGrid->ExplorationSectorRadius);
	NewHash = HashCombineFast(NewHash, GetTypeHash(SectorGrid->GetActorTransform().ToHumanReadableString()));
	for (const FHexSector& Sector : SectorGrid->Sectors)
	{
		NewHash = HashCombineFast(NewHash, HashCombineFast(GetTypeHash(Sector.Id), GetTypeHash(static_cast<uint8>(Sector.State))));
	}
	if (SectorFogMaskTexture && NewHash == SectorFogMaskHash)
	{
		return;
	}

	constexpr int32 MaskResolution = 512;
	if (!SectorFogMaskTexture)
	{
		SectorFogMaskTexture = UTexture2D::CreateTransient(MaskResolution, MaskResolution, PF_B8G8R8A8, TEXT("SectorFogMask"));
		SectorFogMaskTexture->SRGB = false;
		SectorFogMaskTexture->Filter = TF_Nearest;
		SectorFogMaskTexture->AddressX = TA_Clamp;
		SectorFogMaskTexture->AddressY = TA_Clamp;
	}

	FVector2D BoundsMin(FLT_MAX, FLT_MAX);
	FVector2D BoundsMax(-FLT_MAX, -FLT_MAX);
	const float Radius = FMath::Max(100.0f, SectorGrid->ExplorationSectorRadius);
	for (const FHexSector& Sector : SectorGrid->Sectors)
	{
		BoundsMin.X = FMath::Min(BoundsMin.X, Sector.WorldCenter.X - Radius);
		BoundsMin.Y = FMath::Min(BoundsMin.Y, Sector.WorldCenter.Y - Radius);
		BoundsMax.X = FMath::Max(BoundsMax.X, Sector.WorldCenter.X + Radius);
		BoundsMax.Y = FMath::Max(BoundsMax.Y, Sector.WorldCenter.Y + Radius);
	}
	const FVector2D BoundsSize = BoundsMax - BoundsMin;

	FTexture2DMipMap& Mip = SectorFogMaskTexture->GetPlatformData()->Mips[0];
	uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < MaskResolution; ++Y)
	{
		for (int32 X = 0; X < MaskResolution; ++X)
		{
			const FVector2D UV((X + 0.5f) / MaskResolution, (Y + 0.5f) / MaskResolution);
			const FVector2D WorldXY = BoundsMin + UV * BoundsSize;
			const int32 SectorId = SectorGrid->GetSectorAtWorldLocation(FVector(WorldXY, SectorGrid->GetActorLocation().Z));
			const uint8 Opacity = FMath::RoundToInt(FMath::Clamp(SectorGrid->GetFogOpacityForSector(SectorId), 0.0f, 1.0f) * 255.0f);
			const int32 PixelIndex = (Y * MaskResolution + X) * 4;
			Pixels[PixelIndex + 0] = Opacity;
			Pixels[PixelIndex + 1] = Opacity;
			Pixels[PixelIndex + 2] = Opacity;
			Pixels[PixelIndex + 3] = 255;
		}
	}
	Mip.BulkData.Unlock();
	SectorFogMaskTexture->UpdateResource();

	FogOfWarMaterialInstance->SetTextureParameterValue(TEXT("SectorFogMask"), SectorFogMaskTexture);
	FogOfWarMaterialInstance->SetVectorParameterValue(TEXT("FogMaskWorldMin"), FLinearColor(BoundsMin.X, BoundsMin.Y, 0.0f, 0.0f));
	FogOfWarMaterialInstance->SetVectorParameterValue(TEXT("FogMaskWorldSize"), FLinearColor(BoundsSize.X, BoundsSize.Y, 0.0f, 0.0f));
	SectorFogMaskHash = NewHash;
}

void ASurviveThePlanetCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ASurviveThePlanetCameraPawn::MoveForwardPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &ASurviveThePlanetCameraPawn::MoveForwardReleased);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ASurviveThePlanetCameraPawn::MoveBackwardPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &ASurviveThePlanetCameraPawn::MoveBackwardReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ASurviveThePlanetCameraPawn::MoveRightPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &ASurviveThePlanetCameraPawn::MoveRightReleased);
	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ASurviveThePlanetCameraPawn::MoveLeftPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &ASurviveThePlanetCameraPawn::MoveLeftReleased);

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASurviveThePlanetCameraPawn::RotateClockwisePressed);
	PlayerInputComponent->BindKey(EKeys::E, IE_Released, this, &ASurviveThePlanetCameraPawn::RotateClockwiseReleased);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ASurviveThePlanetCameraPawn::RotateCounterClockwisePressed);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Released, this, &ASurviveThePlanetCameraPawn::RotateCounterClockwiseReleased);

	PlayerInputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &ASurviveThePlanetCameraPawn::ZoomCamera);

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn input bound: Pawn=%s Controller=%s InputComponent=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetController()),
		*GetNameSafe(PlayerInputComponent));
}

void ASurviveThePlanetCameraPawn::MoveForwardPressed()
{
	KeyboardPanInput.Y += 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveForwardReleased()
{
	KeyboardPanInput.Y -= 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveBackwardPressed()
{
	KeyboardPanInput.Y -= 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveBackwardReleased()
{
	KeyboardPanInput.Y += 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveRightPressed()
{
	KeyboardPanInput.X += 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveRightReleased()
{
	KeyboardPanInput.X -= 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveLeftPressed()
{
	KeyboardPanInput.X -= 1.0f;
}

void ASurviveThePlanetCameraPawn::MoveLeftReleased()
{
	KeyboardPanInput.X += 1.0f;
}

void ASurviveThePlanetCameraPawn::RotateClockwisePressed()
{
	KeyboardRotationInput += 1.0f;
}

void ASurviveThePlanetCameraPawn::RotateClockwiseReleased()
{
	KeyboardRotationInput -= 1.0f;
}

void ASurviveThePlanetCameraPawn::RotateCounterClockwisePressed()
{
	KeyboardRotationInput -= 1.0f;
}

void ASurviveThePlanetCameraPawn::RotateCounterClockwiseReleased()
{
	KeyboardRotationInput += 1.0f;
}

void ASurviveThePlanetCameraPawn::ZoomCamera(float ZoomInput)
{
	if (FMath::IsNearlyZero(ZoomInput))
	{
		return;
	}

	CameraBoom->TargetArmLength = FMath::Clamp(
		CameraBoom->TargetArmLength - (ZoomInput * CameraZoomSpeed),
		MinCameraZoom,
		MaxCameraZoom);
}

void ASurviveThePlanetCameraPawn::SelectUnderCursor()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn SelectUnderCursor: no PlayerController."));
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const bool bHasMousePosition = PlayerController->GetMousePosition(MouseX, MouseY);
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn SelectUnderCursor: Controller=%s Mouse=(%.1f, %.1f) HasMouse=%s"),
		*GetNameSafe(PlayerController),
		MouseX,
		MouseY,
		bHasMousePosition ? TEXT("true") : TEXT("false"));

	FHitResult Hit;
	const bool bHit = PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, Hit);
	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn trace: bHit=%s HitActor=%s HitClass=%s HitComponent=%s HitLocation=%s IsSelectable=%s"),
		bHit ? TEXT("true") : TEXT("false"),
		*GetNameSafe(HitActor),
		HitActor ? *HitActor->GetClass()->GetPathName() : TEXT("None"),
		*GetNameSafe(Hit.GetComponent()),
		*Hit.Location.ToCompactString(),
		IsSelectableActor(HitActor) ? TEXT("true") : TEXT("false"));

	if (IsSelectableActor(HitActor))
	{
		SetSelectedActor(HitActor);
		return;
	}

	if (bHasMousePosition)
	{
		if (AActor* ScreenActor = FindSelectableActorNearScreenPosition(FVector2D(MouseX, MouseY)))
		{
			SetSelectedActor(ScreenActor);
			return;
		}
	}

	if (bHit)
	{
		if (AActor* NearbyActor = FindSelectableActorNearLocation(Hit.Location))
		{
			SetSelectedActor(NearbyActor);
			return;
		}
	}

	SetSelectedActor(nullptr);
	LogSelectableActors(TEXT("No selection"));
}

FVector2D ASurviveThePlanetCameraPawn::GetEdgeScrollInput() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return FVector2D::ZeroVector;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	if (ViewportWidth <= 0 || ViewportHeight <= 0 || !PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return FVector2D::ZeroVector;
	}

	FVector2D EdgeInput = FVector2D::ZeroVector;
	if (MouseX <= EdgeScrollZone)
	{
		EdgeInput.X -= 1.0f;
	}
	else if (MouseX >= ViewportWidth - EdgeScrollZone)
	{
		EdgeInput.X += 1.0f;
	}

	if (MouseY <= EdgeScrollZone)
	{
		EdgeInput.Y += 1.0f;
	}
	else if (MouseY >= ViewportHeight - EdgeScrollZone)
	{
		EdgeInput.Y -= 1.0f;
	}

	return EdgeInput;
}

void ASurviveThePlanetCameraPawn::PanCamera(const FVector2D& PanInput, float DeltaSeconds)
{
	if (PanInput.IsNearlyZero())
	{
		return;
	}

	const FRotator YawRotation(0.0f, CameraBoom->GetComponentRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector MoveDirection = ((Forward * PanInput.Y) + (Right * PanInput.X)).GetSafeNormal();

	AddActorWorldOffset(MoveDirection * CameraPanSpeed * DeltaSeconds, false);
}

void ASurviveThePlanetCameraPawn::RotateCamera(float RotationInput, float DeltaSeconds)
{
	if (FMath::IsNearlyZero(RotationInput))
	{
		return;
	}

	FRotator CameraRotation = CameraBoom->GetRelativeRotation();
	CameraRotation.Yaw += RotationInput * CameraRotationSpeed * DeltaSeconds;
	CameraBoom->SetRelativeRotation(CameraRotation);
}

bool ASurviveThePlanetCameraPawn::IsSelectableActor(const AActor* Actor) const
{
	return Actor
		&& (Actor->ActorHasTag(TEXT("Drone")) || Actor->ActorHasTag(TEXT("BaseModule")));
}

AActor* ASurviveThePlanetCameraPawn::FindSelectableActorNearLocation(const FVector& Location) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* ClosestActor = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* CandidateActor = *ActorIterator;
		if (!IsSelectableActor(CandidateActor))
		{
			continue;
		}

		FVector Origin = FVector::ZeroVector;
		FVector BoxExtent = FVector::ZeroVector;
		CandidateActor->GetActorBounds(false, Origin, BoxExtent);

		const FVector2D CandidateLocation2D(Origin.X, Origin.Y);
		const FVector2D ClickLocation2D(Location.X, Location.Y);
		const float CandidateRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) + SelectionRingPadding;
		const float DistanceSquared = FVector2D::DistSquared(CandidateLocation2D, ClickLocation2D);
		const float AllowedDistanceSquared = FMath::Square(FMath::Max(SelectionClickRadius, CandidateRadius));

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn world candidate: Actor=%s Class=%s Distance=%.1f Allowed=%.1f Origin=%s BoxExtent=%s"),
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			FMath::Sqrt(DistanceSquared),
			FMath::Sqrt(AllowedDistanceSquared),
			*Origin.ToCompactString(),
			*BoxExtent.ToCompactString());

		if (DistanceSquared <= AllowedDistanceSquared && DistanceSquared < ClosestDistanceSquared)
		{
			ClosestActor = CandidateActor;
			ClosestDistanceSquared = DistanceSquared;
		}
	}

	return ClosestActor;
}

AActor* ASurviveThePlanetCameraPawn::FindSelectableActorNearScreenPosition(const FVector2D& ScreenPosition) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		return nullptr;
	}

	AActor* ClosestActor = nullptr;
	float ClosestDistanceSquared = FMath::Square(SelectionClickRadius);

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* CandidateActor = *ActorIterator;
		if (!IsSelectableActor(CandidateActor))
		{
			continue;
		}

		FVector Origin = FVector::ZeroVector;
		FVector BoxExtent = FVector::ZeroVector;
		CandidateActor->GetActorBounds(false, Origin, BoxExtent);

		FVector2D CandidateScreenPosition = FVector2D::ZeroVector;
		if (!PlayerController->ProjectWorldLocationToScreen(Origin, CandidateScreenPosition, true))
		{
			UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn screen candidate: Actor=%s projection failed."), *GetNameSafe(CandidateActor));
			continue;
		}

		const float CandidateRadius = FMath::Max(FMath::Max(BoxExtent.X, BoxExtent.Y) * 0.75f, SelectionRingMinRadius);
		const float AllowedDistanceSquared = FMath::Square(FMath::Max(SelectionClickRadius, CandidateRadius));
		const float DistanceSquared = FVector2D::DistSquared(CandidateScreenPosition, ScreenPosition);

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn screen candidate: Actor=%s Class=%s ActorScreen=(%.1f, %.1f) ClickScreen=(%.1f, %.1f) Distance=%.1f Allowed=%.1f"),
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			CandidateScreenPosition.X,
			CandidateScreenPosition.Y,
			ScreenPosition.X,
			ScreenPosition.Y,
			FMath::Sqrt(DistanceSquared),
			FMath::Sqrt(AllowedDistanceSquared));

		if (DistanceSquared <= AllowedDistanceSquared && DistanceSquared < ClosestDistanceSquared)
		{
			ClosestActor = CandidateActor;
			ClosestDistanceSquared = DistanceSquared;
		}
	}

	return ClosestActor;
}

void ASurviveThePlanetCameraPawn::SetSelectedActor(AActor* NewSelectedActor)
{
	if (SelectedActor == NewSelectedActor)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn selection unchanged: %s"), *GetNameSafe(SelectedActor));
		return;
	}

	SetActorSelectedVisual(SelectedActor, false);
	SelectedActor = NewSelectedActor;
	SetActorSelectedVisual(SelectedActor, true);

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn selection changed to: %s Class=%s"),
		*GetNameSafe(SelectedActor),
		SelectedActor ? *SelectedActor->GetClass()->GetPathName() : TEXT("None"));
}

void ASurviveThePlanetCameraPawn::SetActorSelectedVisual(AActor* Actor, bool bSelected) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetRenderCustomDepth(bSelected);
		PrimitiveComponent->SetCustomDepthStencilValue(bSelected ? 1 : 0);
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn visual %s: Actor=%s Component=%s Collision=%d VisibilityResponse=%d"),
			bSelected ? TEXT("ON") : TEXT("OFF"),
			*GetNameSafe(Actor),
			*GetNameSafe(PrimitiveComponent),
			static_cast<int32>(PrimitiveComponent->GetCollisionEnabled()),
			static_cast<int32>(PrimitiveComponent->GetCollisionResponseToChannel(ECC_Visibility)));
	}
}

void ASurviveThePlanetCameraPawn::DrawSelectedActorRing() const
{
	if (!IsValid(SelectedActor))
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	SelectedActor->GetActorBounds(false, Origin, BoxExtent);

	const float RingRadius = FMath::Max(FMath::Max(BoxExtent.X, BoxExtent.Y) + SelectionRingPadding, SelectionRingMinRadius);
	const FVector ActorLocation = SelectedActor->GetActorLocation();
	const FVector RingCenter(Origin.X, Origin.Y, ActorLocation.Z + 8.0f);

	DrawDebugCircle(
		GetWorld(),
		RingCenter,
		RingRadius,
		96,
		FColor::Green,
		false,
		0.0f,
		0,
		SelectionRingThickness,
		FVector::ForwardVector,
		FVector::RightVector,
		false);
}

void ASurviveThePlanetCameraPawn::LogSelectableActors(const TCHAR* Reason) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 SelectableCount = 0;
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* CandidateActor = *ActorIterator;
		if (!IsSelectableActor(CandidateActor))
		{
			continue;
		}

		++SelectableCount;
		FVector Origin = FVector::ZeroVector;
		FVector BoxExtent = FVector::ZeroVector;
		CandidateActor->GetActorBounds(false, Origin, BoxExtent);

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn %s selectable[%d]: Actor=%s Class=%s Tags=%s Origin=%s BoxExtent=%s"),
			Reason,
			SelectableCount,
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			*FString::JoinBy(CandidateActor->Tags, TEXT(","), [](const FName& Tag) { return Tag.ToString(); }),
			*Origin.ToCompactString(),
			*BoxExtent.ToCompactString());
	}

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT CameraPawn %s selectable count=%d"), Reason, SelectableCount);
}
