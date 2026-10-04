#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EnergyConnectionComponent.generated.h"

class UStaticMesh;
class UPrimitiveComponent;
class UInstancedStaticMeshComponent;
class USplineMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UEnergyCoverageComponent;
class APlanetSurfaceManager;

/** Decorative connection; does not change power, occupancy or placement validation. */
UCLASS(ClassGroup=(Energy), meta=(BlueprintSpawnableComponent))
class SURVIVETHEPLANET_API UEnergyConnectionComponent : public USceneComponent
{
 GENERATED_BODY()
public:
 UEnergyConnectionComponent();
 void UpdateRoute(bool bPreview, bool bValidPlacement);
 void CopyPlacedRoute(const UEnergyConnectionComponent* Preview);
 AActor* GetSourceActor() const;
 UEnergyCoverageComponent* GetSourceCoverage() const { return Source.Get(); }
 bool HasUsableConnection() const { return !bIsPreview && ActiveCableCount > 0 && Source.IsValid(); }
 int32 GetPoleCount() const { return ActivePoleCount; }
 int32 GetRouteRebuildCount() const { return RouteRebuildCount; }

 UPROPERTY(EditAnywhere, Category="Energy Connection", meta=(ClampMin="100", Units="cm"))
 float PoleSpacing = 600.0f;
 UPROPERTY(EditAnywhere, Category="Energy Connection", meta=(ClampMin="75", Units="cm"))
 float BuildingClearance = 125.0f;
 UPROPERTY(EditAnywhere, Category="Energy Connection", meta=(ClampMin="0", Units="cm"))
 float CableSag = 35.0f;
 UPROPERTY(EditAnywhere, Category="Energy Connection", meta=(ClampMin="0", Units="cm"))
 float SourceSwitchMargin = 150.0f;
 UPROPERTY(EditAnywhere, Category="Energy Connection")
 TObjectPtr<UStaticMesh> PoleMesh;
 UPROPERTY(EditAnywhere, Category="Energy Connection")
 TObjectPtr<UStaticMesh> CableMesh;
 UPROPERTY(EditAnywhere, Category="Energy Connection")
 TObjectPtr<UStaticMesh> TerminalMesh;

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 void BuildRoute();
 void ApplyAppearance(bool bPreview, bool bValidPlacement);
 void SourcesChanged();
 void BindSurface();
 void BuildingsChanged();
 FVector GetAttachment(AActor* Actor, const FVector& Toward) const;
 FVector GroundPoint(const FVector& Point) const;
 void ConfigureVisual(UPrimitiveComponent* Component);
 UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Poles;
 UPROPERTY(Transient) TArray<TObjectPtr<USplineMeshComponent>> Cables;
 UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Terminals;
 UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GhostMaterial;
 UPROPERTY() TObjectPtr<UMaterialInterface> GhostMaterialBase;
 TWeakObjectPtr<UEnergyCoverageComponent> Source;
 TArray<FVector> RoutePoints;
 FIntVector LastPosition = FIntVector(MAX_int32);
 uint32 LastSourceRevision = MAX_uint32;
 FDelegateHandle SourceChangeHandle;
 FDelegateHandle BuildingChangeHandle;
 TWeakObjectPtr<APlanetSurfaceManager> Surface;
 bool bIsPreview = true;
 bool bLastAppearancePreview = false;
 bool bLastAppearanceValid = false;
 int32 ActivePoleCount = 0;
 int32 ActiveCableCount = 0;
 int32 RouteRebuildCount = 0;
};
