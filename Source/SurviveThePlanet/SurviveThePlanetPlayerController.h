// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
//#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/BuildTools/BuildToolTypes.h"
#include "SurviveThePlanetPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UPathFollowingComponent;
class USpringArmComponent;
class ASurviveThePlanetCharacter;
class ASelectableWorldActor;
class AEnergyModule;
class AMiningMachine;
class ABaseResourceSource;
class AResourceManager;
class APlanetSurfaceManager;
class ACableNetworkManager;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildToolChangedSignature, ESTPBuildTool, NewBuildTool);

/**
 * Player controller for a top-down perspective game.
 */
UCLASS()
class ASurviveThePlanetPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	/** Component used for moving along a NavMesh path. */
	UPROPERTY(VisibleDefaultsOnly, Category = AI)
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, Category="Input")
	float ShortPressThreshold;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	/** MappingContext */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	/** Mouse click input action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationClickAction;

	/** Touch input action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationTouchAction;

	/** Camera movement speed in world units per second. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float CameraPanSpeed = 1200.0f;

	/** Screen edge size in pixels that starts mouse edge scrolling. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float EdgeScrollZone = 24.0f;

	/** Spring arm zoom speed in world units per mouse wheel step. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float CameraZoomSpeed = 160.0f;

	/** Closest allowed camera zoom. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinCameraZoom = 600.0f;

	/** Furthest allowed camera zoom. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxCameraZoom = 2600.0f;

	/** Camera rotation speed in degrees per second. */
	UPROPERTY(EditAnywhere, Category="Camera Controls", meta=(ClampMin="0.0", UIMin="0.0"))
	float CameraRotationSpeed = 90.0f;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Set to true if we're using touch input */
	uint32 bIsTouch : 1;

	/** Saved location of the character movement destination */
	FVector CachedDestination;

	/** Time that the click input has been pressed */
	float FollowTime = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Selection")
	TObjectPtr<AActor> SelectedActor;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SelectionRingPadding = 35.0f;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SelectionRingMinRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float SelectionRingThickness = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Selection", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float SelectionClickRadius = 500.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Build Tools")
	ESTPBuildTool ActiveBuildTool = ESTPBuildTool::None;

public:
	/** Constructor */
	ASurviveThePlanetPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Build Tools")
	void SetActiveBuildTool(ESTPBuildTool NewBuildTool);

	UFUNCTION(BlueprintPure, Category = "Build Tools")
	ESTPBuildTool GetActiveBuildTool() const { return ActiveBuildTool; }

	UFUNCTION(BlueprintCallable, Category = "Build Tools")
	void SetEnergyModuleClass(TSubclassOf<AEnergyModule> NewEnergyModuleClass);

	UFUNCTION(BlueprintCallable, Category = "Build Tools")
	void SetMiningMachineClass(TSubclassOf<AMiningMachine> NewMiningMachineClass);

	UPROPERTY(BlueprintAssignable, Category = "Build Tools")
	FBuildToolChangedSignature OnBuildToolChanged;

protected:
	virtual void BeginPlay() override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Update camera controls every frame. */
	virtual void PlayerTick(float DeltaTime) override;
	
	/** Input handlers */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	void OnCancelBuildToolPressed();

	/** Helper function to get the move destination */
	void UpdateCachedDestination();

private:
	UPROPERTY(Transient)
	TSubclassOf<AEnergyModule> EnergyModuleClass;

	UPROPERTY(Transient)
	TObjectPtr<AEnergyModule> EnergyModulePlacementPreview;

	UPROPERTY(Transient)
	TSubclassOf<AMiningMachine> MiningMachineClass;

	UPROPERTY(Transient)
	TObjectPtr<AMiningMachine> MiningMachinePlacementPreview;

	/** Last emitted preview diagnostic; avoids writing the same result every frame. */
	FString LastMiningPlacementDiagnostic;

	UPROPERTY(Transient)
	TObjectPtr<ACableNetworkManager> CableNetworkManager;

	APlanetSurfaceManager* FindPlanetSurfaceManager() const;
	ACableNetworkManager* FindOrCreateCableNetworkManager();
	bool BeginCableDragAtCursor();
	bool UpdateCableDragAtCursor();
	void EndCableDrag();

	ASurviveThePlanetCharacter* GetControlledSurviveCharacter() const;
	USpringArmComponent* GetControlledCameraBoom() const;
	bool TryHandleActiveBuildToolClick();
	bool TryPlaceEnergyModuleAtCursor();
	bool TryPlaceMiningMachineAtCursor();
	void UpdateBuildPlacementPreview();
	void UpdateEnergyModulePlacementPreview();
	void UpdateMiningMachinePlacementPreview();
	void EnsureEnergyModulePlacementPreview();
	void EnsureMiningMachinePlacementPreview();
	void DestroyBuildPlacementPreview();
	void ConfigureEnergyModulePlacementPreview(AEnergyModule* PreviewActor) const;
	void ConfigureMiningMachinePlacementPreview(AMiningMachine* PreviewActor) const;
	ABaseResourceSource* GetResourceSourceUnderCursor(FHitResult* OutHit = nullptr) const;
	AResourceManager* FindResourceManager() const;
	bool TrySelectActorUnderCursor();
	bool TrySelectActorAtScreenPosition(const FVector2D& ScreenPosition);
	AActor* FindSelectableActorNearLocation(const FVector& Location) const;
	AActor* FindSelectableActorNearScreenPosition(const FVector2D& ScreenPosition) const;
	bool IsSelectableActor(const AActor* Actor) const;
	bool TryGetCursorWorldLocation(FVector& OutWorldLocation) const;
	void SetSelectedActor(AActor* NewSelectedActor);
	void SetActorSelectedVisual(AActor* Actor, bool bSelected) const;
	void DrawSelectedActorRing() const;
	void LogSelectableActors(const TCHAR* Reason) const;
	void UpdateCameraControls(float DeltaTime);
	void PanCamera(const FVector2D& PanInput, float DeltaTime);
	void ZoomCamera(float ZoomInput);
	void RotateCamera(float CameraRotationInput, float DeltaTime);
};
