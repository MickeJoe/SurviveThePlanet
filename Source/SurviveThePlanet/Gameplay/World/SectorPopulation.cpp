#include "Gameplay/World/SectorPopulation.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Planet/PlanetSurfaceManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Gameplay/World/Authoring/PlanetSectorTemplate.h"
#include "Gameplay/World/Authoring/PlanetTerrainClusterVariant.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "SceneView.h"

ASectorPopulation::ASectorPopulation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ASectorPopulation::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimerForNextTick(this, &ASectorPopulation::InitializePopulation);
}

void ASectorPopulation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The camera moves while simulation is paused (global dilation 0.0001).
	// A gameplay timer would freeze streaming and leave visible sectors empty.
	const double Now = GetWorld()->GetRealTimeSeconds();
	if (Now >= NextClusterResidencyUpdate)
	{
		NextClusterResidencyUpdate = Now + 0.1;
		UpdateClusterResidency();
	}
}

bool ASectorPopulation::GroundPosition(FVector Position, FVector& Ground) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SectorPopulationGround), true, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Position + FVector(0,0,10000), Position - FVector(0,0,20000), ECC_Visibility, Params)) return false;
	if (!Cast<APlanetSurfaceManager>(Hit.GetActor())) return false;
	Ground = Hit.ImpactPoint;
	return Hit.ImpactNormal.Z > 0.75f;
}

void ASectorPopulation::InitializePopulation()
{
	if (bInitialized) return;
	if (!Grid) for (TActorIterator<AHexSectorGrid> It(GetWorld()); It; ++It) { Grid = *It; break; }
	if (!Grid || Grid->Sectors.IsEmpty()) { Diagnostics.Add(TEXT("No generated sector grid found.")); return; }
	UPlanetResourceDistribution* Input = NewObject<UPlanetResourceDistribution>(this);
	const EResourceType Types[] = {EResourceType::Iron, EResourceType::Copper, EResourceType::Stone};
	for (const FHexSector& Sector : Grid->Sectors)
	{
		FPlanetResourceRule Rule;
		Rule.Id = FName(*FString::Printf(TEXT("Sector_%d"), Sector.Id));
		Rule.ResourceType = Types[Sector.Id % 3]; Rule.SlotType = TEXT("Any");
		Rule.MinCount = Rule.MaxCount = 1;
		Rule.MinQuantity = 3000; Rule.MaxQuantity = 5000; Rule.Radius = 200;
		Rule.AllowedSectorIds.Add(Sector.Id);
		Input->Rules.Add(Rule);
	}
	Resources = UPlanetResourcePlacementComponent::PlaceResources(Grid, Input, Seed);
	if (!Resources.bSuccess) { Diagnostics.Append(Resources.Errors); UE_LOG(LogTemp, Error, TEXT("SECTOR_POPULATION: %s"), *FString::Join(Diagnostics,TEXT("; "))); return; }
	for (auto& Deposit : Resources.Deposits)
	{
		FVector Ground;
		if (!DepositClasses.Contains(Deposit.ResourceType) || !DepositClasses[Deposit.ResourceType]
			|| !GroundPosition(Deposit.Location, Ground))
		{
			Diagnostics.Add(FString::Printf(TEXT("Sector %d: missing deposit class or valid terrain under resource slot."), Deposit.SectorId));
			continue;
		}
		Deposit.Location = Ground;
	}
	if (!Diagnostics.IsEmpty()) { Resources.bSuccess = false; UE_LOG(LogTemp, Error, TEXT("SECTOR_POPULATION: %s"), *FString::Join(Diagnostics,TEXT("; "))); return; }
	if (AuthoredSectorTemplate || ClusterVariantLibrary)
	{
		if (!GenerateAuthoredClusters())
		{
			UE_LOG(LogTemp, Error, TEXT("SECTOR_POPULATION: %s"), *FString::Join(Diagnostics, TEXT("; ")));
			return;
		}
	}
	else if (!GenerateLegacyDecorations()) return;
	bInitialized = true;
	Grid->OnSectorStateChanged.AddDynamic(this, &ASectorPopulation::OnSectorChanged);
	for (const auto& Sector : Grid->Sectors) if (Sector.State != ESectorState::Undiscovered) RevealSector(Sector.Id);
	if (!GeneratedClusters.IsEmpty())
	{
		UpdateClusterResidency();
		SetActorTickEnabled(true);
	}
	UE_LOG(LogTemp, Display, TEXT("SECTOR_POPULATION: planned %d deposits, %d legacy decorations, %d authored clusters across %d sectors."),
		Resources.Deposits.Num(), Decorations.Num(), GeneratedClusters.Num(), Grid->Sectors.Num());
}

bool ASectorPopulation::GenerateAuthoredClusters()
{
	if (!AuthoredSectorTemplate || !ClusterVariantLibrary
		|| !FMath::IsNearlyEqual(AuthoredSectorTemplate->SectorRadius, Grid->ExplorationSectorRadius))
	{
		Diagnostics.Add(TEXT("Authored clusters require a template and library, with a radius matching the sector grid."));
		return false;
	}
	for (const FPlanetSectorClusterSlot& Slot : AuthoredSectorTemplate->ClusterSlots)
	{
		if (ClusterVariantLibrary->FindCompatible(Slot.Shape).IsEmpty())
		{
			Diagnostics.Add(FString::Printf(TEXT("No compatible cluster for slot %s."), *Slot.SlotId.ToString()));
			return false;
		}
	}
	APlanetSurfaceManager* Surface = nullptr;
	for (TActorIterator<APlanetSurfaceManager> It(GetWorld()); It; ++It) { Surface = *It; break; }
	if (!Surface)
	{
		Diagnostics.Add(TEXT("No planet surface for authored clusters."));
		return false;
	}
	// Resolve every sector's floor before spawning anything. Trace the surface
	// actor itself so the HQ or a resource actor cannot obscure the ground.
	TArray<FTransform> Transforms;
	for (const FHexSector& Sector : Grid->Sectors)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(AuthoredClusterGround), true);
		if (!Surface->ActorLineTraceSingle(Hit, Sector.WorldCenter + FVector(0,0,10000),
			Sector.WorldCenter - FVector(0,0,20000), ECC_Visibility, Params))
		{
			Diagnostics.Add(FString::Printf(TEXT("No ground under sector %d."), Sector.Id));
			return false;
		}
		Transforms.Add(FTransform(Grid->GetActorQuat(), Hit.ImpactPoint, Grid->GetActorScale3D()));
	}
	for (int32 Index = 0; Index < Grid->Sectors.Num(); ++Index)
	{
		const FHexSector& Sector = Grid->Sectors[Index];
		APlanetGeneratedSector* Generated = GetWorld()->SpawnActorDeferred<APlanetGeneratedSector>(
			APlanetGeneratedSector::StaticClass(), Transforms[Index], this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Generated)
		{
			for (auto& Pair : GeneratedClusters) if (IsValid(Pair.Value)) Pair.Value->Destroy();
			GeneratedClusters.Reset();
			Diagnostics.Add(TEXT("Failed to spawn an authored sector."));
			return false;
		}
		Generated->SectorTemplate = AuthoredSectorTemplate;
		Generated->VariantLibrary = ClusterVariantLibrary;
		Generated->Seed = static_cast<int32>((static_cast<uint32>(Seed) ^ (static_cast<uint32>(Sector.Id) * 7919u)) & 0x7fffffffu);
		Generated->bDeferRuntimeGeneration = bManageClusterResidency;
		Generated->bOptimizeRendering = bOptimizeClusterRendering;
		// Construction generates ISMs after the authoritative slot data is set.
		Generated->FinishSpawning(Transforms[Index]);
		Generated->SetActorHiddenInGame(Sector.State == ESectorState::Undiscovered);
		GeneratedClusters.Add(Sector.Id, Generated);
	}
	return true;
}

void ASectorPopulation::UpdateClusterResidency()
{
	if (!bInitialized || !Grid || GeneratedClusters.IsEmpty()) return;
	FConvexVolume ViewFrustum;
	FVector ViewOrigin = FVector::ZeroVector;
	bool bHasView = false;
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		ULocalPlayer* Player = Controller->GetLocalPlayer();
		FSceneViewProjectionData Projection;
		if (Player && Player->ViewportClient && Player->GetProjectionData(Player->ViewportClient->Viewport, Projection))
		{
			GetViewFrustumBounds(ViewFrustum, Projection.ComputeViewProjectionMatrix(), false);
			ViewOrigin = Projection.ViewOrigin;
			bHasView = true;
		}
	}
	const double Now = GetWorld()->GetRealTimeSeconds();
	TArray<APlanetGeneratedSector*> Pending;
	const double Radius = Grid->ExplorationSectorRadius * Grid->GetActorScale3D().GetAbsMax() + FMath::Max(0.0f, ClusterPrefetchMargin);
	for (const FHexSector& Sector : Grid->Sectors)
	{
		APlanetGeneratedSector* Generated = GeneratedClusters.FindRef(Sector.Id);
		if (!IsValid(Generated)) continue;
		const bool bDiscovered = Sector.State != ESectorState::Undiscovered;
		// Without a view (startup/headless) retain discovered sectors rather than hide them.
		const bool bNeeded = !bManageClusterResidency || (bDiscovered && (!bHasView
			|| ViewFrustum.IntersectSphere(Generated->GetActorLocation(), Radius)));
		Generated->SetActorHiddenInGame(!bDiscovered);
		if (bNeeded)
		{
			ClusterLastNeededTime.Add(Sector.Id, Now);
			if (!Generated->bResident) Pending.Add(Generated);
		}
		else if (Generated->bResident && (!bDiscovered
			|| Now - ClusterLastNeededTime.FindRef(Sector.Id) > FMath::Max(0.0f, ClusterUnloadDelay)))
		{
			Generated->ClearGenerated();
		}
	}
	Pending.Sort([&ViewOrigin](const APlanetGeneratedSector& A, const APlanetGeneratedSector& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), ViewOrigin) < FVector::DistSquared(B.GetActorLocation(), ViewOrigin);
	});
	for (int32 Index = 0; Index < FMath::Min(Pending.Num(), FMath::Max(1, ClusterLoadsPerUpdate)); ++Index)
	{
		Pending[Index]->Generate();
	}
}

bool ASectorPopulation::GenerateLegacyDecorations()
{
	const int32 LandmarkEnd = FMath::Clamp(LandmarkMeshCount, 1, DecorationMeshes.Num());
	const int32 RockEnd = FMath::Clamp(LandmarkEnd + RockMeshCount, LandmarkEnd, DecorationMeshes.Num());
	if (RockEnd >= DecorationMeshes.Num())
	{
		Diagnostics.Add(TEXT("DecorationMeshes must contain landmarks, rocks and at least one plant mesh."));
		Resources.bSuccess = false;
		return false;
	}
	for (const FHexSector& Sector : Grid->Sectors)
	{
		FSectorTemplateDefinition Template;
		if (!Grid->GetTemplateForSector(Sector.Id, Template)) continue;
		FRandomStream Random(Seed ^ (Sector.Id * 7919));
		auto IsProtected = [&Template](const FVector2D& Local, float Radius)
		{
			for (const auto& Pocket : Template.BuildablePockets)
			{
				const FVector2D D = (Local - Pocket.Center).GetAbs();
				if (D.X < Pocket.Extent.X + Radius && D.Y < Pocket.Extent.Y + Radius) return true;
			}
			for (const auto& Corridor : Template.DroneCorridors)
			{
				const FVector2D D = Corridor.End - Corridor.Start;
				const double T = D.SizeSquared() > 0 ? FMath::Clamp(FVector2D::DotProduct(Local - Corridor.Start,D)/D.SizeSquared(),0.0,1.0) : 0;
				if (FVector2D::Distance(Local, Corridor.Start + D*T) < Radius + Corridor.Width * 0.5f) return true;
			}
			return false;
		};
		TArray<FVector2D> CoveragePoints;
		const int32 SamplesPerAxis = FMath::Clamp(CoverageSamplesPerAxis, 12, 64);
		for (int32 SampleY = 0; SampleY < SamplesPerAxis; ++SampleY)
		{
			for (int32 SampleX = 0; SampleX < SamplesPerAxis; ++SampleX)
			{
				const FVector2D Local(
					FMath::Lerp(-Grid->ExplorationSectorRadius, Grid->ExplorationSectorRadius, (SampleX + 0.5f) / SamplesPerAxis),
					FMath::Lerp(-Grid->ExplorationSectorRadius, Grid->ExplorationSectorRadius, (SampleY + 0.5f) / SamplesPerAxis));
				FVector2D Rotated = Local;
				if (Sector.bTemplateMirrored) Rotated.X *= -1;
				Rotated = Rotated.GetRotated(Sector.TemplateRotationDegrees);
				const FVector WorldPosition = Sector.WorldCenter + Grid->GetActorTransform().TransformVectorNoScale(FVector(Rotated, 0));
				if (Grid->GetSectorAtWorldLocation(WorldPosition) == Sector.Id) CoveragePoints.Add(Local);
			}
		}
		// Coverage is measured from sampled sector ground, independent of instance count.
		TSet<int32> CoveredSamples;
		auto MarkCoverage = [&CoveragePoints, &CoveredSamples](const FVector2D& Local, float Radius)
		{
			const float RadiusSquared = FMath::Square(Radius);
			for (int32 SampleIndex = 0; SampleIndex < CoveragePoints.Num(); ++SampleIndex)
				if (FVector2D::DistSquared(CoveragePoints[SampleIndex], Local) <= RadiusSquared) CoveredSamples.Add(SampleIndex);
		};
		auto GetCoverage = [&CoveragePoints, &CoveredSamples]()
		{
			return CoveragePoints.IsEmpty() ? 0.0f : static_cast<float>(CoveredSamples.Num()) / CoveragePoints.Num();
		};
		auto TryAdd = [&](const FVector2D& Local, int32 MeshIndex, float Scale, float SeparationScale)
		{
			if (!DecorationMeshes.IsValidIndex(MeshIndex)) return false;
			UStaticMesh* Mesh = DecorationMeshes[MeshIndex];
			if (!Mesh) return false;
			// Imported meshes have different native sizes: constrain world height, not a shared scale multiplier.
			const float NativeHeight = Mesh->GetBoundingBox().GetSize().Z;
			const float HeightScale = MeshIndex < LandmarkEnd && NativeHeight > 1.0f
				? FMath::Min(Scale, 360.0f / NativeHeight) : Scale;
			const float Radius = Mesh->GetBounds().BoxExtent.Size2D() * Scale;
			if (IsProtected(Local, Radius)) return false;
			FVector2D Rotated = Local;
			if (Sector.bTemplateMirrored) Rotated.X *= -1;
			Rotated = Rotated.GetRotated(Sector.TemplateRotationDegrees);
			FVector Position = Sector.WorldCenter + Grid->GetActorTransform().TransformVectorNoScale(FVector(Rotated,0));
			if (Grid->GetSectorAtWorldLocation(Position) != Sector.Id) return false;
			for (const auto& Deposit : Resources.Deposits)
				if (FVector::Dist2D(Position, Deposit.Location) < Radius + Deposit.Radius + 100) return false;
			for (const auto& Existing : Decorations)
				if (Existing.SectorId == Sector.Id && FVector::Dist2D(Position, Existing.Transform.GetLocation()) < (Radius + Existing.Radius) * SeparationScale) return false;
			if (!GroundPosition(Position, Position)) return false;
			Position.Z -= Mesh->GetBoundingBox().Min.Z * HeightScale;
			FSectorDecoration Decoration;
			Decoration.SectorId = Sector.Id; Decoration.MeshIndex = MeshIndex; Decoration.Radius = Radius;
			Decoration.Transform = FTransform(FRotator(0,Random.FRandRange(0,360),0), Position, FVector(Scale, Scale, HeightScale));
			Decorations.Add(Decoration);
			// Coverage represents structural terrain. Decorative vegetation is
			// deliberately excluded so colourful ground cover does not consume build area.
			if (MeshIndex < RockEnd) MarkCoverage(Local, Radius * 0.9f);
			return true;
		};

		TArray<FVector2D> FormationAnchors;
		const float SectorVariation = Random.FRandRange(-TerrainCoverageVariation, TerrainCoverageVariation);
		float TargetCoverage = FMath::Clamp(TerrainCoverageTarget + SectorVariation, 0.05f, 0.75f);
		if (Sector.Id == Grid->StartingSectorId) TargetCoverage *= StartSectorCoverageScale;
		auto AddFormationStamp = [&](const FVector2D& Anchor, float DirectionAngle, float StampScale)
		{
			const FVector2D Direction(FMath::Cos(DirectionAngle), FMath::Sin(DirectionAngle));
			const FVector2D Perpendicular(-Direction.Y, Direction.X);
			// A stamp is an authored-looking low ridge with two planted shoulders.
			// Cliff meshes dominate; tall Expedition04 pillars are rare accents.
			const bool bAccent = Random.FRand() < 0.08f && LandmarkEnd > 3;
			const int32 PrimaryMesh = bAccent ? Random.RandRange(3, LandmarkEnd - 1)
				: Random.RandRange(0, FMath::Min(2, LandmarkEnd - 1));
			if (!TryAdd(Anchor, PrimaryMesh, Random.FRandRange(0.78f, 1.08f), 0.48f)) return false;
			const int32 RidgePieces = StampScale > 1.0f ? Random.RandRange(7, 9) : Random.RandRange(4, 6);
			for (int32 Piece = 1; Piece < RidgePieces; ++Piece)
			{
				const float Side = Piece % 2 == 0 ? -1.0f : 1.0f;
				const int32 AlongIndex = FMath::CeilToInt(Piece * 0.5f);
				const float Along = AlongIndex * Random.FRandRange(340.0f, 450.0f) * Side * StampScale;
				const float RowOffset = StampScale > 1.0f && Piece % 3 == 0
					? Random.FRandRange(300.0f, 480.0f) * (Piece % 2 == 0 ? -1.0f : 1.0f) : 0.0f;
				const FVector2D Position = Anchor + Direction * Along
					+ Perpendicular * (RowOffset + Random.FRandRange(-150.0f, 150.0f) * StampScale);
				TryAdd(Position, Random.RandRange(0, FMath::Min(2, LandmarkEnd - 1)),
					Random.FRandRange(0.68f, 1.0f), 0.45f);
				MarkCoverage(Position, 500.0f * StampScale);
			}
			for (int32 Index = 0; Index < 14; ++Index)
			{
				const float Along = Random.FRandRange(-820.0f, 820.0f) * StampScale;
				const float Across = Random.FRandRange(-520.0f, 520.0f) * StampScale;
				TryAdd(Anchor + Direction * Along + Perpendicular * Across,
					Random.RandRange(LandmarkEnd, RockEnd - 1), Random.FRandRange(0.28f, 0.68f), 0.2f);
			}
			for (int32 Index = 0; Index < 32; ++Index)
			{
				const float Shoulder = Index % 2 == 0 ? -1.0f : 1.0f;
				const float Along = Random.FRandRange(-920.0f, 920.0f) * StampScale;
				const float Across = Shoulder * Random.FRandRange(280.0f, 720.0f) * StampScale;
				// Prefer the vivid Expedition02 plants; use Expedition06 ground cover sparingly.
				const int32 PlantEnd = FMath::Min(RockEnd + 3, DecorationMeshes.Num() - 1);
				const int32 PlantMesh = Random.FRand() < 0.92f
					? Random.RandRange(RockEnd, PlantEnd)
					: Random.RandRange(RockEnd, DecorationMeshes.Num() - 1);
				TryAdd(Anchor + Direction * Along + Perpendicular * Across,
					PlantMesh, Random.FRandRange(0.82f, 1.42f), 0.13f);
			}
			MarkCoverage(Anchor, 580.0f * StampScale);
			return true;
		};

		// Explicit asymmetric stamp slots replace the former circular distribution.
		// Wide outer formations frame the sector; shorter inner seams separate bays.
		struct FFormationSlot
		{
			FVector2D Position;
			float DirectionDegrees;
			float Scale;
		};
		const float LayoutScale = Grid->ExplorationSectorRadius / 4000.0f;
		// Candidates are interleaved by biome. If one route blocks an anchor, the
		// following alternatives preserve the four-sided composition.
		const TArray<FFormationSlot> OuterSlots = {
			{{-900.0f, 2820.0f}, 4.0f, 1.12f}, {{2920.0f, 250.0f}, 82.0f, 1.10f},
			{{1350.0f, -2920.0f}, 8.0f, 1.10f}, {{-2920.0f, 250.0f}, 98.0f, 1.10f},
			{{1500.0f, 2850.0f}, -12.0f, 1.08f}, {{2780.0f, -1150.0f}, 68.0f, 1.08f},
			{{-1350.0f, -2920.0f}, -8.0f, 1.10f}, {{-2780.0f, -1150.0f}, 112.0f, 1.08f},
			{{-1500.0f, 2850.0f}, 12.0f, 1.06f}, {{2550.0f, 700.0f}, 92.0f, 1.05f},
			{{2400.0f, -2600.0f}, -34.0f, 1.08f}, {{-2550.0f, 700.0f}, 88.0f, 1.05f}};
		const TArray<FFormationSlot> InnerSlots = {
			{{-1500.0f, -300.0f}, 58.0f, 0.92f},
			{{1450.0f, -350.0f}, 58.0f, 0.92f},
			{{-1500.0f, 300.0f}, -58.0f, 0.88f},
			{{1500.0f, 350.0f}, -58.0f, 0.88f}};
		// Coverage changes the physical breadth of the six authored regions rather
		// than adding fragments around a ring.
		const float CoverageScale = FMath::Clamp(TargetCoverage / 0.40f, 0.72f, 1.28f);
		auto AddSlots = [&](const TArray<FFormationSlot>& Slots, int32 DesiredCount)
		{
			int32 Added = 0;
			for (int32 SlotIndex = 0; SlotIndex < Slots.Num() && Added < DesiredCount; ++SlotIndex)
			{
				const FFormationSlot& Slot = Slots[SlotIndex];
				const FVector2D Candidate = Slot.Position * LayoutScale;
				if (IsProtected(Candidate, 140.0f)) continue;
				if (AddFormationStamp(Candidate, FMath::DegreesToRadians(Slot.DirectionDegrees), Slot.Scale * CoverageScale))
				{
					FormationAnchors.Add(Candidate);
					++Added;
				}
			}
		};
		AddSlots(OuterSlots, 8);
		AddSlots(InnerSlots, 2);
		const float ActualCoverage = GetCoverage();
		ActualTerrainCoverage.Add(Sector.Id, ActualCoverage);
		if (ActualCoverage + 0.03f < TargetCoverage)
			Diagnostics.Add(FString::Printf(TEXT("Sector %d reached %.1f%% of %.1f%% target terrain coverage."),
				Sector.Id, ActualCoverage * 100.0f, TargetCoverage * 100.0f));
	}
	return true;
}

void ASectorPopulation::OnSectorChanged(int32 SectorId, ESectorState State)
{
	if (State != ESectorState::Undiscovered) RevealSector(SectorId);
}

void ASectorPopulation::RevealSector(int32 SectorId)
{
	if (!bInitialized || LoadedSectors.Contains(SectorId)) return;
	LoadedSectors.Add(SectorId);
	for (const auto& Deposit : Resources.Deposits)
	{
		if (Deposit.SectorId != SectorId) continue;
		const auto Class = DepositClasses.FindRef(Deposit.ResourceType);
		ABaseResourceSource* Actor = GetWorld()->SpawnActorDeferred<ABaseResourceSource>(Class, FTransform(Deposit.Location), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Actor) { Diagnostics.Add(TEXT("Failed to spawn a planned resource deposit.")); continue; }
		Actor->ConfigureGeneratedDeposit(Deposit.Quantity);
		Actor->bRevealedBySector = true;
		Actor->FinishSpawning(FTransform(Deposit.Location));
		SpawnedDeposits.Add(Actor);
	}
	if (APlanetGeneratedSector* Generated = GeneratedClusters.FindRef(SectorId))
	{
		Generated->SetActorHiddenInGame(false);
		return;
	}
	for (int32 MeshIndex = 0; MeshIndex < DecorationMeshes.Num(); ++MeshIndex)
	{
		if (!DecorationMeshes[MeshIndex]) continue;
		auto* Group = NewObject<UInstancedStaticMeshComponent>(this);
		Group->SetupAttachment(GetRootComponent()); Group->SetStaticMesh(DecorationMeshes[MeshIndex]);
		if (DecorationMaterialSets.IsValidIndex(MeshIndex))
		{
			const TArray<TObjectPtr<UMaterialInterface>>& Materials = DecorationMaterialSets[MeshIndex].Materials;
			for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
				if (Materials[MaterialIndex]) Group->SetMaterial(MaterialIndex, Materials[MaterialIndex]);
		}
		// Dressing is visual. Reserved pockets and corridors govern gameplay space.
		Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Group->RegisterComponent(); InstanceGroups.Add(Group);
		for (const auto& Decoration : Decorations)
			if (Decoration.SectorId == SectorId && Decoration.MeshIndex == MeshIndex) Group->AddInstance(Decoration.Transform, true);
	}
}

void ASectorPopulation::EndPlay(const EEndPlayReason::Type Reason)
{
	SetActorTickEnabled(false);
	if (Grid) Grid->OnSectorStateChanged.RemoveDynamic(this, &ASectorPopulation::OnSectorChanged);
	for (ABaseResourceSource* Deposit : SpawnedDeposits) if (IsValid(Deposit)) Deposit->Destroy();
	for (auto& Pair : GeneratedClusters) if (IsValid(Pair.Value)) Pair.Value->Destroy();
	GeneratedClusters.Reset();
	Super::EndPlay(Reason);
}
