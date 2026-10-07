#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Gameplay/Energy/EnergyCoverageSubsystem.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Gameplay/Energy/EnergyConnectionRouting.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UEnergyConnectionComponent::UEnergyConnectionComponent()
{
 PrimaryComponentTick.bCanEverTick = false;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Pole(TEXT("/Game/Units/Buildings/PowerConnection/SM_PowerPole.SM_PowerPole"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cable(TEXT("/Game/Units/Buildings/PowerConnection/SM_PowerCableSpan.SM_PowerCableSpan"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Terminal(TEXT("/Game/Units/Buildings/PowerConnection/SM_PowerCableTerminal.SM_PowerCableTerminal"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ghost(TEXT("/Game/UI/Materials/M_BuildPlacement.M_BuildPlacement"));
 PoleMesh=Pole.Object; CableMesh=Cable.Object; TerminalMesh=Terminal.Object; GhostMaterialBase=Ghost.Object;
}
void UEnergyConnectionComponent::BeginPlay()
{
 Super::BeginPlay();
 if (UEnergyCoverageSubsystem* System=GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>())
  SourceChangeHandle=System->OnSourcesChanged.AddUObject(this,&UEnergyConnectionComponent::SourcesChanged);
 const ABaseBuilding* Building = Cast<ABaseBuilding>(GetOwner());
 UpdateRoute(Building && Building->IsPlacementPreview(),true);
}
void UEnergyConnectionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
 if (UEnergyCoverageSubsystem* System=GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>())
  System->OnSourcesChanged.Remove(SourceChangeHandle);
 if (Poles) Poles->DestroyComponent();
 for (USplineMeshComponent* Cable:Cables) if (Cable) Cable->DestroyComponent();
 for (UStaticMeshComponent* Terminal:Terminals) if (Terminal) Terminal->DestroyComponent();
 Cables.Reset(); Terminals.Reset(); Poles=nullptr;
 if (Surface.IsValid()) Surface->OnBuildingOccupancyChanged.Remove(BuildingChangeHandle);
 Super::EndPlay(Reason);
}
AActor* UEnergyConnectionComponent::GetSourceActor() const { return Source.IsValid()?Source->GetOwner():nullptr; }
float UEnergyConnectionComponent::GetConnectionDistanceMeters() const
{
 AActor* Parent = GetSourceActor();
 if (!Parent || !GetOwner()) return 0.0f;
 return FVector::Dist2D(GetAttachment(Parent, GetOwner()->GetActorLocation()),
  GetAttachment(GetOwner(), Parent->GetActorLocation())) / 100.0f;
}

void UEnergyConnectionComponent::SourcesChanged()
{
 // Fixed parent after placement prevents completed extenders from forming cycles.
 if (!bIsPreview && !GetWorld()->bIsTearingDown && (!Source.IsValid() || !Source->IsEligibleSource()))
  UpdateRoute(false,true);
}
void UEnergyConnectionComponent::ConfigureVisual(UPrimitiveComponent* Component)
{
 Component->SetupAttachment(this);
 Component->SetAbsolute(true,true,true);
 Component->SetMobility(EComponentMobility::Movable);
 Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Component->SetGenerateOverlapEvents(false);
 Component->SetReceivesDecals(false);
 Component->SetCanEverAffectNavigation(false);
 Component->RegisterComponent();
 Component->SetWorldTransform(FTransform::Identity);
}
FVector UEnergyConnectionComponent::GroundPoint(const FVector& Point) const
{
 FCollisionQueryParams Params(SCENE_QUERY_STAT(EnergyPoleGround),false,GetOwner());
 if (GetSourceActor()) Params.AddIgnoredActor(GetSourceActor());
 FHitResult Hit;
 if (GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,1500),Point-FVector(0,0,2500),ECC_Visibility,Params))
  return Hit.ImpactPoint;
 FVector Fallback = Point;
 if (auto* Mesh = GetOwner()->FindComponentByClass<UStaticMeshComponent>()) Fallback.Z = Mesh->Bounds.GetBox().Min.Z;
 return Fallback;
}
FVector UEnergyConnectionComponent::GetAttachment(AActor* Actor,const FVector& Toward) const
{
 UStaticMeshComponent* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();
 const FBox MeshBounds=Mesh?Mesh->Bounds.GetBox():FBox(Actor->GetActorLocation()-FVector(100),Actor->GetActorLocation()+FVector(100));
 const FVector Direction=(Toward-Actor->GetActorLocation()).GetSafeNormal2D();
 const ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);
 const UBuildingDataAsset* Definition = Building ? Building->GetBuildingData() : nullptr;
 const bool bExtender = Definition && Definition->BuildTool == ESTPBuildTool::EnergyExtender;
 FVector Point=MeshBounds.GetCenter();
 Point+=Direction*(bExtender?55.0f:FMath::Max(MeshBounds.GetExtent().X,MeshBounds.GetExtent().Y)*0.8f);
 Point.Z=bExtender?MeshBounds.Min.Z+255.0f:MeshBounds.Max.Z;
 if (!bExtender && Mesh && Mesh->GetStaticMesh())
 {
  // Bounds include rooftop dishes. Trace at the chosen edge to mount on the roof itself.
  FCollisionQueryParams Params(SCENE_QUERY_STAT(EnergyRoofTerminal),true,GetOwner());
  FHitResult RoofHit;
  const FVector TraceStart(Point.X,Point.Y,MeshBounds.Max.Z+100.0f);
  const FVector TraceEnd(Point.X,Point.Y,MeshBounds.Min.Z-100.0f);
  if (GetWorld()->LineTraceSingleByChannel(RoofHit,TraceStart,TraceEnd,ECC_Visibility,Params)
   && RoofHit.GetComponent() == Mesh)
   Point.Z = RoofHit.ImpactPoint.Z;
 }
 return Point;
}
void UEnergyConnectionComponent::BindSurface()
{
 if (Surface.IsValid()) return;
 for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It)
 {
  Surface = *It;
  BuildingChangeHandle = It->OnBuildingOccupancyChanged.AddUObject(this, &UEnergyConnectionComponent::BuildingsChanged);
  break;
 }
}
void UEnergyConnectionComponent::BuildingsChanged()
{
 if (GetWorld()->bIsTearingDown) return;
 LastPosition = FIntVector(MAX_int32);
 UpdateRoute(bIsPreview, bLastAppearanceValid);
}
void UEnergyConnectionComponent::UpdateRoute(bool bPreview,bool bValidPlacement)
{
 BindSurface();
 bIsPreview=bPreview;
 UEnergyCoverageSubsystem* System=GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>();
 if (!System) return;
 const FVector Location=GetOwner()->GetActorLocation();
 const FIntVector Position(FMath::RoundToInt(Location.X),FMath::RoundToInt(Location.Y),FMath::RoundToInt(Location.Z));
 if (Position!=LastPosition || System->GetSourceRevision()!=LastSourceRevision)
 {
  LastPosition=Position; LastSourceRevision=System->GetSourceRevision();
  UEnergyCoverageComponent* Current = Source.Get();
  // A valid placed connection retains its parent; previews choose the nearest source.
  if (bPreview || !Current || !Current->IsEligibleSource())
   Source=System->FindNearestSource(Location,GetOwner(),Current,SourceSwitchMargin);
  BuildRoute();
 }
 ApplyAppearance(bPreview,bValidPlacement);
 if (!bPreview) System->RefreshPoweredCoverageVisualization();
}
void UEnergyConnectionComponent::CopyPlacedRoute(const UEnergyConnectionComponent* Preview)
{
 if (!Preview) return;
 BindSurface();
 Source=Preview->Source;
 BuildingClearance=Preview->BuildingClearance;
 PoleSpacing=Preview->PoleSpacing; CableSag=Preview->CableSag;
 bIsPreview=false;
 // Both ghost and final building use the same grid location, ground pivot and endpoint calculation.
 BuildRoute();
 ApplyAppearance(false,true);
 if (auto* System = GetWorld()->GetSubsystem<UEnergyCoverageSubsystem>()) System->RefreshPoweredCoverageVisualization();
}
void UEnergyConnectionComponent::BuildRoute()
{
 ++RouteRebuildCount; ActivePoleCount=0; ActiveCableCount=0; RoutePoints.Reset();
 AActor* Parent=GetSourceActor();
 if (!Parent || !PoleMesh || !CableMesh)
 {
  if (Poles) Poles->SetVisibility(false);
  for (USplineMeshComponent* Cable : Cables) Cable->SetVisibility(false);
  for (UStaticMeshComponent* Terminal : Terminals) Terminal->SetVisibility(false);
  return;
 }
 const FVector StartBase=GetAttachment(Parent,GetOwner()->GetActorLocation());
 const FVector EndBase=GetAttachment(GetOwner(),Parent->GetActorLocation());
 const FVector Start=StartBase+FVector(0,0,51), End=EndBase+FVector(0,0,51);
 TMap<AActor*, FBox> ReservedBounds;
 if (Surface.IsValid()) Surface->GetBuildingRoutingBounds(ReservedBounds);
 TArray<FBox2D> Obstacles;
 for (const auto& Entry : ReservedBounds)
 {
  if (Entry.Key == Parent || Entry.Key == GetOwner()) continue;
  Obstacles.Add(FBox2D(FVector2D(Entry.Value.Min), FVector2D(Entry.Value.Max)).ExpandBy(FMath::Max(75.0f, BuildingClearance)));
 }
 if (Surface.IsValid())
 {
  for (const FBox& TerrainBounds : Surface->GetTerrainRoutingBounds())
  {
   const float Margin = FMath::Max(75.0f, BuildingClearance);
   const FBox2D ClusterBox = FBox2D(FVector2D(TerrainBounds.Min), FVector2D(TerrainBounds.Max)).ExpandBy(Margin);
   if (!ClusterBox.IsInside(FVector2D(Start)) && !ClusterBox.IsInside(FVector2D(End)))
   {
    Obstacles.Add(ClusterBox);
    continue;
   }
   // A building can stand in a free recess of an irregular cluster. Resolve that cluster
   // with its actual mesh footprints rather than trapping the terminal in a coarse box.
   for (const FBox& MeshBounds : Surface->GetTerrainMeshRoutingBounds())
   {
    const FBox2D MeshBox(FVector2D(MeshBounds.Min), FVector2D(MeshBounds.Max));
    if (ClusterBox.Intersect(MeshBox)) Obstacles.Add(MeshBox.ExpandBy(Margin));
   }
  }
 }
 TArray<FVector2D> Path;
 bool bHasPath = FEnergyConnectionRouting::FindPath(FVector2D(Start), FVector2D(End), Obstacles, Path);
 int32 RequiredPoles = -1;
 for (int32 Leg = 0; Leg + 1 < Path.Num(); ++Leg)
  RequiredPoles += FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(Path[Leg], Path[Leg+1]) / FMath::Max(100.0f, PoleSpacing)));
 bHasPath &= RequiredPoles <= 64;
 // Hide an unroutable decorative link rather than block construction or cross a building.
 if (!bHasPath)
 {
  if (Poles) Poles->SetVisibility(false);
  for (USplineMeshComponent* Cable : Cables) Cable->SetVisibility(false);
  for (UStaticMeshComponent* Terminal : Terminals) Terminal->SetVisibility(false);
  return;
 }
 if (!Poles)
 {
  Poles=NewObject<UInstancedStaticMeshComponent>(GetOwner());
  Poles->SetStaticMesh(PoleMesh);
  ConfigureVisual(Poles);
 }
 Poles->SetVisibility(true);
 const FRotator Rotation=(End-Start).GetSafeNormal2D().Rotation();
 RoutePoints.Add(Start);
 for (int32 Leg = 0; Leg + 1 < Path.Num(); ++Leg)
 {
  const FVector2D A = Path[Leg], B = Path[Leg+1];
  const int32 SpanCount = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(A, B) / FMath::Max(100.0f, PoleSpacing)));
  // Every bend gets a pole. Cables stay within each clear straight leg.
  for (int32 Index = 1; Index <= SpanCount; ++Index)
  {
   if (Leg + 2 == Path.Num() && Index == SpanCount) continue;
   const FVector2D Point = FMath::Lerp(A, B, float(Index)/SpanCount);
   FVector Ground = GroundPoint(FVector(Point, Start.Z));
   FVector2D Direction = (B-A).GetSafeNormal();
   if (Index == SpanCount && Leg + 2 < Path.Num())
    Direction = (Direction + (Path[Leg+2]-B).GetSafeNormal()).GetSafeNormal();
   const FTransform PoleTransform(FVector(Direction, 0).Rotation(), Ground);
   if (ActivePoleCount < Poles->GetInstanceCount())
    Poles->UpdateInstanceTransform(ActivePoleCount, PoleTransform, true, false, true);
   else
    Poles->AddInstance(PoleTransform, true);
   RoutePoints.Add(Ground + FVector(0,0,327));
   ++ActivePoleCount;
  }
 }
 for (int32 Index = ActivePoleCount; Index < Poles->GetInstanceCount(); ++Index)
  Poles->UpdateInstanceTransform(Index,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,false,true);
 Poles->MarkRenderStateDirty();
 RoutePoints.Add(End);
 if(TerminalMesh)
 {
  while (Terminals.Num() < 2)
  {
   UStaticMeshComponent* Terminal = NewObject<UStaticMeshComponent>(GetOwner());
   Terminal->SetStaticMesh(TerminalMesh);
   ConfigureVisual(Terminal);
   Terminals.Add(Terminal);
  }
  Terminals[0]->SetWorldTransform(FTransform(Rotation,StartBase));Terminals[1]->SetWorldTransform(FTransform(Rotation,EndBase));
  for(UStaticMeshComponent* Terminal:Terminals)Terminal->SetVisibility(true);
 }
 constexpr int32 PiecesPerSpan=4;
 for(int32 Span=0;Span<RoutePoints.Num()-1;++Span)
 {
  const FVector A=RoutePoints[Span], B=RoutePoints[Span+1];
  const float Sag=FMath::Min(CableSag,FVector::Dist2D(A,B)*0.08f);
  auto NodeTangent = [&](int32 Node)
  {
   FVector Direction = (B-A).GetSafeNormal2D();
   if (Node > 0 && Node + 1 < RoutePoints.Num())
    Direction = ((RoutePoints[Node]-RoutePoints[Node-1]).GetSafeNormal2D()
     + (RoutePoints[Node+1]-RoutePoints[Node]).GetSafeNormal2D()).GetSafeNormal2D();
   FVector Result = Direction * FVector::Dist2D(A, B);
   Result.Z = B.Z - A.Z;
   return Result;
  };
  const FVector StartTangent = NodeTangent(Span), EndTangent = NodeTangent(Span+1);
  auto Point = [&](float T)
  {
   return FMath::CubicInterp(A, StartTangent, B, EndTangent, T) - FVector(0,0,4*Sag*T*(1-T));
  };
  auto Tangent = [&](float T)
  {
   return FMath::CubicInterpDerivative(A, StartTangent, B, EndTangent, T) - FVector(0,0,4*Sag*(1-2*T));
  };
  for(int32 Piece=0;Piece<PiecesPerSpan;++Piece)
  {
   if(!Cables.IsValidIndex(ActiveCableCount))
   {
    USplineMeshComponent* Cable=NewObject<USplineMeshComponent>(GetOwner());
    Cable->SetStaticMesh(CableMesh);
    Cable->SetForwardAxis(ESplineMeshAxis::X,false);
    ConfigureVisual(Cable);
    Cables.Add(Cable);
   }
   USplineMeshComponent* Cable=Cables[ActiveCableCount++];
   const float T0=float(Piece)/PiecesPerSpan,T1=float(Piece+1)/PiecesPerSpan;
   Cable->SetStartAndEnd(Point(T0),Tangent(T0)/PiecesPerSpan,Point(T1),Tangent(T1)/PiecesPerSpan,false);
   // Narrow only the endpoints to meet the compact building terminals.
   auto Width = [&](float T)
   {
    float Value = 1.0f;
    if (Span == 0) Value *= FMath::Lerp(0.25f,1.0f,T);
    if (Span == RoutePoints.Num()-2) Value *= FMath::Lerp(1.0f,0.25f,T);
    return Value;
   };
   const float StartWidth=Width(T0), EndWidth=Width(T1);
   Cable->SetStartScale(FVector2D(StartWidth,1),false);
   Cable->SetEndScale(FVector2D(EndWidth,1),false);
   // Scale setters skip their update when unchanged. Moving a pooled segment still requires one.
   Cable->UpdateMesh();
   Cable->UpdateBounds();
   Cable->SetVisibility(true);
  }
 }
 for(int32 Index=ActiveCableCount;Index<Cables.Num();++Index)Cables[Index]->SetVisibility(false);
 // A reused pool may contain newly allocated components; reapply material state.
 bLastAppearancePreview=!bIsPreview;
}
void UEnergyConnectionComponent::ApplyAppearance(bool bPreview,bool bValidPlacement)
{
 if(bLastAppearancePreview==bPreview && bLastAppearanceValid==bValidPlacement)return;
 bLastAppearancePreview=bPreview;bLastAppearanceValid=bValidPlacement;
 auto Appearance = [&](UStaticMeshComponent* Mesh)
 {
  if (!Mesh || !Mesh->GetStaticMesh()) return;
  // Keep the ivory/orange asset palette in placement mode; use an outline for validity.
  Mesh->SetCastShadow(true);
  Mesh->SetRenderCustomDepth(bPreview);
  Mesh->SetCustomDepthStencilValue(bValidPlacement ? 2 : 3);
  for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
   Mesh->SetMaterial(Slot,Mesh->GetStaticMesh()->GetMaterial(Slot));
 };
 Appearance(Poles);
 for(USplineMeshComponent* Cable:Cables)Appearance(Cable);
 for(UStaticMeshComponent* Terminal:Terminals)Appearance(Terminal);
}
