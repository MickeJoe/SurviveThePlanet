// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurviveThePlanetPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "NiagaraSystem.h"
#include "SurviveThePlanetCameraPawn.h"
#include "SurviveThePlanetCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Navigation/PathFollowingComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/SelectableWorldActor.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Work/ConstructionJobQueueSubsystem.h"
#include "SurviveThePlanet.h"

ASurviveThePlanetPlayerController::ASurviveThePlanetPlayerController()
{
	bIsTouch = false;
	bMoveToMouseCursor = false;

	// create the path following comp
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("Path Following Component"));

	// configure the controller
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
	ShortPressThreshold = 0.3f;

	DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/TopDown/Input/IMC_Default"));
	SetDestinationClickAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Click"));
	SetDestinationTouchAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TopDown/Input/Actions/IA_SetDestination_Touch"));
	FXCursor = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/TopDown/Cursor/FX_Cursor_Success"));
	EnergyModuleClass = AEnergyModule::StaticClass();
}

void ASurviveThePlanetPlayerController::SetActiveBuildTool(ESTPBuildTool NewBuildTool)
{
	if (ActiveBuildTool == NewBuildTool)
	{
		return;
	}

	if (ActiveBuildTool == ESTPBuildTool::EnergyCable)
	{
		EndCableDrag();
	}

	ActiveBuildTool = NewBuildTool;
	if (ActiveBuildTool != ESTPBuildTool::EnergyModule)
	{
		DestroyBuildPlacementPreview();
	}

	OnBuildToolChanged.Broadcast(ActiveBuildTool);
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Active build tool changed to %d"), static_cast<int32>(ActiveBuildTool));
}

void ASurviveThePlanetPlayerController::SetEnergyModuleClass(TSubclassOf<AEnergyModule> NewEnergyModuleClass)
{
	EnergyModuleClass = NewEnergyModuleClass;
	if (!EnergyModuleClass)
	{
		EnergyModuleClass = AEnergyModule::StaticClass();
	}

	DestroyBuildPlacementPreview();
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD EnergyModuleClass set to %s"), *GetNameSafe(EnergyModuleClass.Get()));
}

void ASurviveThePlanetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	const AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	const FString PawnClassName = GetPawn() ? GetPawn()->GetClass()->GetPathName() : TEXT("None");
	const FString GameModeName = GetNameSafe(GameMode);
	const FString GameModeClassName = GameMode ? GameMode->GetClass()->GetPathName() : TEXT("None");

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT PlayerController BeginPlay: Controller=%s Class=%s Pawn=%s PawnClass=%s GameMode=%s GameModeClass=%s"),
		*GetNameSafe(this),
		*GetClass()->GetPathName(),
		*GetNameSafe(GetPawn()),
		*PawnClassName,
		*GameModeName,
		*GameModeClassName);

	LogSelectableActors(TEXT("BeginPlay"));
}

void ASurviveThePlanetPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASurviveThePlanetPlayerController::OnCancelBuildToolPressed);

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT SetupInputComponent: Controller=%s InputComponent=%s InputComponentClass=%s IsLocal=%s DefaultMappingContext=%s ClickAction=%s TouchAction=%s"),
		*GetNameSafe(this),
		*GetNameSafe(InputComponent),
		InputComponent ? *InputComponent->GetClass()->GetPathName() : TEXT("None"),
		IsLocalPlayerController() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(DefaultMappingContext),
		*GetNameSafe(SetDestinationClickAction),
		*GetNameSafe(SetDestinationTouchAction));

	// Only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Added mapping context: %s"), *GetNameSafe(DefaultMappingContext));
			}
			else
			{
				UE_LOG(LogSurviveThePlanet, Error, TEXT("STP_SELECT DefaultMappingContext is null; click selection input will not fire."));
			}
		}
		else
		{
			UE_LOG(LogSurviveThePlanet, Error, TEXT("STP_SELECT Missing EnhancedInputLocalPlayerSubsystem."));
		}

		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT EnhancedInputComponent found; binding click/touch actions."));

			// Setup mouse input events
			if (SetDestinationClickAction)
			{
				EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ASurviveThePlanetPlayerController::OnInputStarted);
				EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ASurviveThePlanetPlayerController::OnSetDestinationTriggered);
				EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ASurviveThePlanetPlayerController::OnSetDestinationReleased);
				EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ASurviveThePlanetPlayerController::OnSetDestinationReleased);
			}
			else
			{
				UE_LOG(LogSurviveThePlanet, Error, TEXT("STP_SELECT SetDestinationClickAction is null; mouse selection cannot bind."));
			}

			// Setup touch input events
			if (SetDestinationTouchAction)
			{
				EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &ASurviveThePlanetPlayerController::OnInputStarted);
				EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &ASurviveThePlanetPlayerController::OnTouchTriggered);
				EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &ASurviveThePlanetPlayerController::OnTouchReleased);
				EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &ASurviveThePlanetPlayerController::OnTouchReleased);
			}
			else
			{
				UE_LOG(LogSurviveThePlanet, Error, TEXT("STP_SELECT SetDestinationTouchAction is null; touch selection cannot bind."));
			}
		}
		else
		{
			UE_LOG(LogSurviveThePlanet, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
		}
	}
}

void ASurviveThePlanetPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateBuildPlacementPreview();
	DrawSelectedActorRing();
}

void ASurviveThePlanetPlayerController::OnInputStarted()
{
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT OnInputStarted: bIsTouch=%s Pawn=%s"), bIsTouch ? TEXT("true") : TEXT("false"), *GetNameSafe(GetPawn()));

	StopMovement();

	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();

	if (!bIsTouch && ActiveBuildTool == ESTPBuildTool::EnergyCable)
	{
		BeginCableDragAtCursor();
		return;
	}

	if (!bIsTouch)
	{
		if (ActiveBuildTool != ESTPBuildTool::None)
		{
			return;
		}

		TrySelectActorUnderCursor();
	}
}

void ASurviveThePlanetPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// Update the move destination to wherever the cursor is pointing at
	UpdateCachedDestination();

	if (!bIsTouch && ActiveBuildTool == ESTPBuildTool::EnergyCable)
	{
		UpdateCableDragAtCursor();
		return;
	}
	
	// Only a real gameplay character should move toward click/touch destinations.
	ASurviveThePlanetCharacter* ControlledCharacter = GetControlledSurviveCharacter();
	if (ControlledCharacter != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledCharacter->GetActorLocation()).GetSafeNormal();
		ControlledCharacter->AddMovementInput(WorldDirection, 1.0, false);
	}
	else
	{
		UE_LOG(LogSurviveThePlanet, VeryVerbose, TEXT("STP_SELECT Held click ignored for movement because pawn is not SurviveThePlanetCharacter. Pawn=%s"), *GetNameSafe(GetPawn()));
	}
}

void ASurviveThePlanetPlayerController::OnSetDestinationReleased()
{
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT OnSetDestinationReleased: FollowTime=%.3f ShortPressThreshold=%.3f"), FollowTime, ShortPressThreshold);

	if (!bIsTouch && ActiveBuildTool == ESTPBuildTool::EnergyCable)
	{
		EndCableDrag();
		FollowTime = 0.0f;
		return;
	}

	if (FollowTime <= ShortPressThreshold)
	{
		if (TryHandleActiveBuildToolClick())
		{
			FollowTime = 0.f;
			return;
		}

		TrySelectActorUnderCursor();
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
void ASurviveThePlanetPlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void ASurviveThePlanetPlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void ASurviveThePlanetPlayerController::OnCancelBuildToolPressed()
{
	if (ActiveBuildTool == ESTPBuildTool::None)
	{
		return;
	}

	SetActiveBuildTool(ESTPBuildTool::None);
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Placement mode cancelled with Escape."));
}

bool ASurviveThePlanetPlayerController::TryHandleActiveBuildToolClick()
{
	switch (ActiveBuildTool)
	{
	case ESTPBuildTool::EnergyModule:
		return TryPlaceEnergyModuleAtCursor();
	case ESTPBuildTool::EnergyCable:
		return true;
	case ESTPBuildTool::None:
	default:
		return false;
	}
}

bool ASurviveThePlanetPlayerController::TryPlaceEnergyModuleAtCursor()
{
	FVector TargetLocation = FVector::ZeroVector;
	if (!TryGetCursorWorldLocation(TargetLocation))
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: cursor did not hit the world."));
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: no world."));
		return true;
	}

	APlanetSurfaceManager* SurfaceManager = FindPlanetSurfaceManager();
	if (!SurfaceManager)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: no PlanetSurfaceManager found."));
		return true;
	}

	TSubclassOf<AEnergyModule> ClassToSpawn = EnergyModuleClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = AEnergyModule::StaticClass();
	}

	const AEnergyModule* DefaultModule = ClassToSpawn->GetDefaultObject<AEnergyModule>();
	const FIntPoint Footprint = DefaultModule ? DefaultModule->GetGridFootprint() : FIntPoint(2, 2);
	const FSTPGridPlacement Placement = SurfaceManager->GetPlacementForWorldLocation(TargetLocation, Footprint);
	if (!Placement.bValid)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: grid cell is invalid or occupied. OriginCell=(%d,%d) Footprint=(%d,%d)"),
			Placement.OriginCell.X,
			Placement.OriginCell.Y,
			Footprint.X,
			Footprint.Y);
		return true;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnergyModule* SpawnedModule = World->SpawnActor<AEnergyModule>(
		ClassToSpawn,
		Placement.WorldLocation,
		Placement.WorldRotation,
		SpawnParameters);

	if (!SpawnedModule)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: SpawnActor returned null. Class=%s"),
			*GetNameSafe(ClassToSpawn.Get()));
		return true;
	}

	SpawnedModule->SetPlacementPreview(false);
	if (!SurfaceManager->ReserveCells(SpawnedModule, Placement.OriginCell, SpawnedModule->GetGridFootprint()))
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Energy module placement failed: reservation was rejected after spawn."));
		SpawnedModule->Destroy();
		return true;
	}

	SpawnedModule->SetConstructionProgress(0.0f);
	SpawnedModule->ShowConstructionProgress();

	if (UConstructionJobQueueSubsystem* ConstructionJobQueue = World->GetSubsystem<UConstructionJobQueueSubsystem>())
	{
		ConstructionJobQueue->EnqueueConstructionJob(SpawnedModule);
	}

	SetSelectedActor(SpawnedModule);
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Placed energy module %s OriginCell=(%d,%d) Footprint=(%d,%d) Location=%s"),
		*GetNameSafe(SpawnedModule),
		Placement.OriginCell.X,
		Placement.OriginCell.Y,
		SpawnedModule->GetGridFootprint().X,
		SpawnedModule->GetGridFootprint().Y,
		*SpawnedModule->GetActorLocation().ToCompactString());

	return true;
}
void ASurviveThePlanetPlayerController::UpdateBuildPlacementPreview()
{
	switch (ActiveBuildTool)
	{
	case ESTPBuildTool::EnergyModule:
		UpdateEnergyModulePlacementPreview();
		break;
	default:
		DestroyBuildPlacementPreview();
		break;
	}
}

void ASurviveThePlanetPlayerController::UpdateEnergyModulePlacementPreview()
{
	EnsureEnergyModulePlacementPreview();

	if (!IsValid(EnergyModulePlacementPreview))
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, true, Hit);
	if (!bHit)
	{
		EnergyModulePlacementPreview->SetActorHiddenInGame(true);
		return;
	}

	APlanetSurfaceManager* SurfaceManager = FindPlanetSurfaceManager();
	if (!SurfaceManager)
	{
		EnergyModulePlacementPreview->SetPlacementPreviewValid(false);
		EnergyModulePlacementPreview->SetActorLocation(Hit.Location, false);
		EnergyModulePlacementPreview->SetActorHiddenInGame(false);
		return;
	}

	const FSTPGridPlacement Placement = SurfaceManager->GetPlacementForWorldLocation(Hit.Location, EnergyModulePlacementPreview->GetGridFootprint());
	EnergyModulePlacementPreview->SetActorLocation(Placement.WorldLocation, false);
	EnergyModulePlacementPreview->SetActorRotation(Placement.WorldRotation);
	EnergyModulePlacementPreview->SetPlacementPreviewValid(Placement.bValid);
	EnergyModulePlacementPreview->SetActorHiddenInGame(false);
}

void ASurviveThePlanetPlayerController::EnsureEnergyModulePlacementPreview()
{
	if (IsValid(EnergyModulePlacementPreview))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AEnergyModule> ClassToSpawn = EnergyModuleClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = AEnergyModule::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EnergyModulePlacementPreview = World->SpawnActor<AEnergyModule>(
		ClassToSpawn,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (EnergyModulePlacementPreview)
	{
		ConfigureEnergyModulePlacementPreview(EnergyModulePlacementPreview);
		EnergyModulePlacementPreview->SetActorHiddenInGame(true);
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_BUILD Created energy module placement preview %s Class=%s"),
			*GetNameSafe(EnergyModulePlacementPreview),
			*EnergyModulePlacementPreview->GetClass()->GetPathName());
	}
}

void ASurviveThePlanetPlayerController::DestroyBuildPlacementPreview()
{
	if (IsValid(EnergyModulePlacementPreview))
	{
		EnergyModulePlacementPreview->Destroy();
	}

	EnergyModulePlacementPreview = nullptr;
}

void ASurviveThePlanetPlayerController::ConfigureEnergyModulePlacementPreview(AEnergyModule* PreviewActor) const
{
	if (!PreviewActor)
	{
		return;
	}

	PreviewActor->SetPlacementPreview(true);
	PreviewActor->SetActorTickEnabled(false);
}

void ASurviveThePlanetPlayerController::UpdateCachedDestination()
{
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
		UE_LOG(LogSurviveThePlanet, Verbose, TEXT("STP_SELECT UpdateCachedDestination hit Actor=%s Component=%s Location=%s"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			*Hit.Location.ToCompactString());
	}
	else
	{
		UE_LOG(LogSurviveThePlanet, Verbose, TEXT("STP_SELECT UpdateCachedDestination found no hit."));
	}
}

bool ASurviveThePlanetPlayerController::TrySelectActorUnderCursor()
{
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	if (!bIsTouch && GetMousePosition(ScreenPosition.X, ScreenPosition.Y))
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT TrySelectActorUnderCursor mouse screen=(%.1f, %.1f)"), ScreenPosition.X, ScreenPosition.Y);
		return TrySelectActorAtScreenPosition(ScreenPosition);
	}

	FHitResult Hit;
	const bool bHitSuccessful = bIsTouch
		? GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit)
		: GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	AActor* HitActor = bHitSuccessful ? Hit.GetActor() : nullptr;
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Cursor trace fallback: bHit=%s HitActor=%s HitClass=%s HitComponent=%s HitLocation=%s"),
		bHitSuccessful ? TEXT("true") : TEXT("false"),
		*GetNameSafe(HitActor),
		HitActor ? *HitActor->GetClass()->GetPathName() : TEXT("None"),
		*GetNameSafe(Hit.GetComponent()),
		*Hit.Location.ToCompactString());

	if (IsSelectableActor(HitActor))
	{
		SetSelectedActor(HitActor);
		UE_LOG(LogSurviveThePlanet, Log, TEXT("Selected actor: %s"), *GetNameSafe(HitActor));
		return true;
	}

	if (bHitSuccessful)
	{
		if (AActor* NearbyActor = FindSelectableActorNearLocation(Hit.Location))
		{
			SetSelectedActor(NearbyActor);
			UE_LOG(LogSurviveThePlanet, Log, TEXT("Selected actor near click: %s"), *GetNameSafe(NearbyActor));
			return true;
		}
	}

	SetSelectedActor(nullptr);
	return false;
}

bool ASurviveThePlanetPlayerController::TrySelectActorAtScreenPosition(const FVector2D& ScreenPosition)
{
	FHitResult Hit;
	const bool bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	AActor* HitActor = bHitSuccessful ? Hit.GetActor() : nullptr;
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Screen click trace: screen=(%.1f, %.1f) bHit=%s HitActor=%s HitClass=%s HitComponent=%s HitLocation=%s IsSelectable=%s"),
		ScreenPosition.X,
		ScreenPosition.Y,
		bHitSuccessful ? TEXT("true") : TEXT("false"),
		*GetNameSafe(HitActor),
		HitActor ? *HitActor->GetClass()->GetPathName() : TEXT("None"),
		*GetNameSafe(Hit.GetComponent()),
		*Hit.Location.ToCompactString(),
		IsSelectableActor(HitActor) ? TEXT("true") : TEXT("false"));

	if (IsSelectableActor(HitActor))
	{
		SetSelectedActor(HitActor);
		UE_LOG(LogSurviveThePlanet, Log, TEXT("Selected actor: %s"), *GetNameSafe(HitActor));
		return true;
	}

	if (AActor* ScreenActor = FindSelectableActorNearScreenPosition(ScreenPosition))
	{
		SetSelectedActor(ScreenActor);
		UE_LOG(LogSurviveThePlanet, Log, TEXT("Selected actor near cursor: %s"), *GetNameSafe(ScreenActor));
		return true;
	}

	if (bHitSuccessful)
	{
		if (AActor* NearbyActor = FindSelectableActorNearLocation(Hit.Location))
		{
			SetSelectedActor(NearbyActor);
			UE_LOG(LogSurviveThePlanet, Log, TEXT("Selected actor near click: %s"), *GetNameSafe(NearbyActor));
			return true;
		}
	}

	SetSelectedActor(nullptr);
	LogSelectableActors(TEXT("No selection"));
	return false;
}

AActor* ASurviveThePlanetPlayerController::FindSelectableActorNearLocation(const FVector& Location) const
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

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT World candidate: Actor=%s Class=%s Origin=%s BoxExtent=%s Distance=%.1f Allowed=%.1f"),
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			*Origin.ToCompactString(),
			*BoxExtent.ToCompactString(),
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

AActor* ASurviveThePlanetPlayerController::FindSelectableActorNearScreenPosition(const FVector2D& ScreenPosition) const
{
	UWorld* World = GetWorld();
	if (!World)
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
		if (!ProjectWorldLocationToScreen(Origin, CandidateScreenPosition, true))
		{
			continue;
		}

		const float CandidateRadius = FMath::Max(FMath::Max(BoxExtent.X, BoxExtent.Y) * 0.75f, SelectionRingMinRadius);
		const float AllowedDistanceSquared = FMath::Square(FMath::Max(SelectionClickRadius, CandidateRadius));
		const float DistanceSquared = FVector2D::DistSquared(CandidateScreenPosition, ScreenPosition);

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Screen candidate: Actor=%s Class=%s ActorScreen=(%.1f, %.1f) ClickScreen=(%.1f, %.1f) Distance=%.1f Allowed=%.1f Origin=%s BoxExtent=%s"),
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			CandidateScreenPosition.X,
			CandidateScreenPosition.Y,
			ScreenPosition.X,
			ScreenPosition.Y,
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

bool ASurviveThePlanetPlayerController::IsSelectableActor(const AActor* Actor) const
{
	if (const ASelectableWorldActor* SelectableActor = Cast<ASelectableWorldActor>(Actor))
	{
		return SelectableActor->IsWorldSelectable();
	}

	return Actor
		&& Actor->ActorHasTag(TEXT("BaseModule"));
}

bool ASurviveThePlanetPlayerController::TryGetCursorWorldLocation(FVector& OutWorldLocation) const
{
	FHitResult Hit;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, true, Hit);
	if (!bHit)
	{
		return false;
	}

	OutWorldLocation = Hit.Location;
	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_CURSOR target hit: Actor=%s Component=%s Location=%s"),
		*GetNameSafe(Hit.GetActor()),
		*GetNameSafe(Hit.GetComponent()),
		*OutWorldLocation.ToCompactString());

	return true;
}

void ASurviveThePlanetPlayerController::SetSelectedActor(AActor* NewSelectedActor)
{
	if (SelectedActor == NewSelectedActor)
	{
		return;
	}

	SetActorSelectedVisual(SelectedActor, false);
	SelectedActor = NewSelectedActor;
	SetActorSelectedVisual(SelectedActor, true);

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Selection changed to: %s Class=%s"),
		*GetNameSafe(SelectedActor),
		SelectedActor ? *SelectedActor->GetClass()->GetPathName() : TEXT("None"));
}

void ASurviveThePlanetPlayerController::SetActorSelectedVisual(AActor* Actor, bool bSelected) const
{
	if (!IsValid(Actor))
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetRenderCustomDepth(bSelected);
			PrimitiveComponent->SetCustomDepthStencilValue(bSelected ? 1 : 0);
			UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT Visual %s: Actor=%s Component=%s Collision=%d VisibilityResponse=%d"),
				bSelected ? TEXT("ON") : TEXT("OFF"),
				*GetNameSafe(Actor),
				*GetNameSafe(PrimitiveComponent),
				static_cast<int32>(PrimitiveComponent->GetCollisionEnabled()),
				static_cast<int32>(PrimitiveComponent->GetCollisionResponseToChannel(ECC_Visibility)));
		}
	}
}

void ASurviveThePlanetPlayerController::DrawSelectedActorRing() const
{
	if (!IsValid(SelectedActor))
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	SelectedActor->GetActorBounds(false, Origin, BoxExtent);

	const float RingRadius = FMath::Max(
		FMath::Max(BoxExtent.X, BoxExtent.Y) + SelectionRingPadding,
		SelectionRingMinRadius);

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

void ASurviveThePlanetPlayerController::LogSelectableActors(const TCHAR* Reason) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT %s: no world."), Reason);
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

		UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT %s selectable[%d]: Actor=%s Class=%s Tags=%s Origin=%s BoxExtent=%s"),
			Reason,
			SelectableCount,
			*GetNameSafe(CandidateActor),
			*CandidateActor->GetClass()->GetPathName(),
			*FString::JoinBy(CandidateActor->Tags, TEXT(","), [](const FName& Tag) { return Tag.ToString(); }),
			*Origin.ToCompactString(),
			*BoxExtent.ToCompactString());
	}

	UE_LOG(LogSurviveThePlanet, Warning, TEXT("STP_SELECT %s selectable count=%d"), Reason, SelectableCount);
}

APlanetSurfaceManager* ASurviveThePlanetPlayerController::FindPlanetSurfaceManager() const
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
ACableNetworkManager* ASurviveThePlanetPlayerController::FindOrCreateCableNetworkManager()
{
	if (IsValid(CableNetworkManager))
	{
		return CableNetworkManager;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
	{
		CableNetworkManager = *It;
		return CableNetworkManager;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CableNetworkManager = World->SpawnActor<ACableNetworkManager>(
		ACableNetworkManager::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);

	return CableNetworkManager;
}

bool ASurviveThePlanetPlayerController::BeginCableDragAtCursor()
{
	FVector WorldLocation;
	ACableNetworkManager* Manager = FindOrCreateCableNetworkManager();
	return Manager
		&& TryGetCursorWorldLocation(WorldLocation)
		&& Manager->BeginCableDrag(WorldLocation);
}

bool ASurviveThePlanetPlayerController::UpdateCableDragAtCursor()
{
	FVector WorldLocation;
	return IsValid(CableNetworkManager)
		&& TryGetCursorWorldLocation(WorldLocation)
		&& CableNetworkManager->UpdateCableDrag(WorldLocation);
}

void ASurviveThePlanetPlayerController::EndCableDrag()
{
	if (IsValid(CableNetworkManager))
	{
		CableNetworkManager->EndCableDrag();
	}
}

ASurviveThePlanetCharacter* ASurviveThePlanetPlayerController::GetControlledSurviveCharacter() const
{
	return Cast<ASurviveThePlanetCharacter>(GetPawn());
}

USpringArmComponent* ASurviveThePlanetPlayerController::GetControlledCameraBoom() const
{
	if (ASurviveThePlanetCameraPawn* CameraPawn = Cast<ASurviveThePlanetCameraPawn>(GetPawn()))
	{
		return CameraPawn->GetCameraBoom();
	}

	if (ASurviveThePlanetCharacter* ControlledCharacter = GetControlledSurviveCharacter())
	{
		return ControlledCharacter->GetCameraBoom();
	}

	return nullptr;
}

void ASurviveThePlanetPlayerController::UpdateCameraControls(float DeltaTime)
{
	FVector2D PanInput = FVector2D::ZeroVector;

	if (IsInputKeyDown(EKeys::W))
	{
		PanInput.Y += 1.0f;
	}
	if (IsInputKeyDown(EKeys::S))
	{
		PanInput.Y -= 1.0f;
	}
	if (IsInputKeyDown(EKeys::D))
	{
		PanInput.X += 1.0f;
	}
	if (IsInputKeyDown(EKeys::A))
	{
		PanInput.X -= 1.0f;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	GetViewportSize(ViewportWidth, ViewportHeight);

	if (ViewportWidth > 0 && ViewportHeight > 0 && GetMousePosition(MouseX, MouseY))
	{
		if (MouseX <= EdgeScrollZone)
		{
			PanInput.X -= 1.0f;
		}
		else if (MouseX >= ViewportWidth - EdgeScrollZone)
		{
			PanInput.X += 1.0f;
		}

		if (MouseY <= EdgeScrollZone)
		{
			PanInput.Y += 1.0f;
		}
		else if (MouseY >= ViewportHeight - EdgeScrollZone)
		{
			PanInput.Y -= 1.0f;
		}
	}

	PanCamera(PanInput, DeltaTime);
	ZoomCamera(GetInputAnalogKeyState(EKeys::MouseWheelAxis));

	float CameraRotationInput = 0.0f;
	if (IsInputKeyDown(EKeys::Q))
	{
		CameraRotationInput -= 1.0f;
	}
	if (IsInputKeyDown(EKeys::E))
	{
		CameraRotationInput += 1.0f;
	}

	RotateCamera(CameraRotationInput, DeltaTime);
}

void ASurviveThePlanetPlayerController::PanCamera(const FVector2D& PanInput, float DeltaTime)
{
	if (PanInput.IsNearlyZero())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const USpringArmComponent* CameraBoom = GetControlledCameraBoom();
	const float CameraYaw = CameraBoom ? CameraBoom->GetComponentRotation().Yaw : ControlledPawn->GetActorRotation().Yaw;
	const FRotator YawRotation(0.0f, CameraYaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector MoveDirection = ((Forward * PanInput.Y) + (Right * PanInput.X)).GetSafeNormal();

	ControlledPawn->AddActorWorldOffset(MoveDirection * CameraPanSpeed * DeltaTime, false);
}

void ASurviveThePlanetPlayerController::ZoomCamera(float ZoomInput)
{
	if (FMath::IsNearlyZero(ZoomInput))
	{
		return;
	}

	if (USpringArmComponent* CameraBoom = GetControlledCameraBoom())
	{
		CameraBoom->TargetArmLength = FMath::Clamp(
			CameraBoom->TargetArmLength - (ZoomInput * CameraZoomSpeed),
			MinCameraZoom,
			MaxCameraZoom);
	}
}

void ASurviveThePlanetPlayerController::RotateCamera(float CameraRotationInput, float DeltaTime)
{
	if (FMath::IsNearlyZero(CameraRotationInput))
	{
		return;
	}

	if (USpringArmComponent* CameraBoom = GetControlledCameraBoom())
	{
		FRotator CameraRotation = CameraBoom->GetRelativeRotation();
		CameraRotation.Yaw += CameraRotationInput * CameraRotationSpeed * DeltaTime;
		CameraBoom->SetRelativeRotation(CameraRotation);
	}
}
