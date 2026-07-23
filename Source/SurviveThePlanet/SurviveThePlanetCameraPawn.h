// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SurviveThePlanetCameraPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Invisible pawn used as the player's isometric camera rig.
 */
UCLASS()
class ASurviveThePlanetCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ASurviveThePlanetCameraPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UCameraComponent* GetCameraComponent() const { return CameraComponent.Get(); }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

protected:
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

	UPROPERTY(EditAnywhere, Category="Selection", meta=(ClampMin="0.0", UIMin="0.0"))
	float SelectionRingPadding = 35.0f;

	UPROPERTY(EditAnywhere, Category="Selection", meta=(ClampMin="1.0", UIMin="1.0"))
	float SelectionRingMinRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category="Selection", meta=(ClampMin="0.1", UIMin="0.1"))
	float SelectionRingThickness = 6.0f;

	UPROPERTY(EditAnywhere, Category="Selection", meta=(ClampMin="1.0", UIMin="1.0"))
	float SelectionClickRadius = 500.0f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	FVector2D KeyboardPanInput = FVector2D::ZeroVector;
	float KeyboardRotationInput = 0.0f;
	TObjectPtr<AActor> SelectedActor;

	void MoveForwardPressed();
	void MoveForwardReleased();
	void MoveBackwardPressed();
	void MoveBackwardReleased();
	void MoveRightPressed();
	void MoveRightReleased();
	void MoveLeftPressed();
	void MoveLeftReleased();
	void RotateClockwisePressed();
	void RotateClockwiseReleased();
	void RotateCounterClockwisePressed();
	void RotateCounterClockwiseReleased();
	void ZoomCamera(float ZoomInput);
	void SelectUnderCursor();

	FVector2D GetEdgeScrollInput() const;
	void PanCamera(const FVector2D& PanInput, float DeltaSeconds);
	void RotateCamera(float RotationInput, float DeltaSeconds);
	bool IsSelectableActor(const AActor* Actor) const;
	AActor* FindSelectableActorNearLocation(const FVector& Location) const;
	AActor* FindSelectableActorNearScreenPosition(const FVector2D& ScreenPosition) const;
	void SetSelectedActor(AActor* NewSelectedActor);
	void SetActorSelectedVisual(AActor* Actor, bool bSelected) const;
	void DrawSelectedActorRing() const;
	void LogSelectableActors(const TCHAR* Reason) const;
};
