#include "PlanetTerrainClusterVariant.h"
#include "PlanetSectorTemplate.h"
#include "PlanetTerrainClusterShape.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"

TArray<UPlanetTerrainClusterVariant*> UPlanetTerrainClusterLibrary::FindCompatible(const UPlanetTerrainClusterShape* Shape) const
{
	TArray<UPlanetTerrainClusterVariant*> Result;
	if (!Shape) return Result;
	for (UPlanetTerrainClusterVariant* Variant : Variants)
	{
		if (Variant && Variant->CompatibleShape == Shape && !Variant->Elements.IsEmpty()) Result.AddUnique(Variant);
	}
	// Asset order in the catalog must not change seeded selection.
	Result.Sort([](const UPlanetTerrainClusterVariant& A, const UPlanetTerrainClusterVariant& B)
	{
		return A.GetPathName() < B.GetPathName();
	});
	return Result;
}

APlanetGeneratedSector::APlanetGeneratedSector()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	GetRootComponent()->SetMobility(EComponentMobility::Static);
}

void APlanetGeneratedSector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	bPlacementBoundsReady = false;
	GetPlacementMeshBounds();
	if (!bDeferRuntimeGeneration) Generate();
}

void APlanetGeneratedSector::ClearGenerated()
{
	for (UInstancedStaticMeshComponent* Group : InstanceGroups)
	{
		if (IsValid(Group)) Group->DestroyComponent();
	}
	InstanceGroups.Reset();
	SelectedVariantIds.Reset();
	Diagnostics.Reset();
	bResident = false;
}

void APlanetGeneratedSector::AddComposition(const UPlanetTerrainClusterVariant* Variant, const FTransform& SlotTransform)
{
	for (const FPlanetClusterElement& Element : Variant->Elements)
	{
		if (!Element.Mesh) continue;
		const FTransform InstanceTransform = Element.Transform * SlotTransform;
		// Size tiers use placed geometry, not asset names. Keep structural silhouettes.
		const FVector Extent = Element.Mesh->GetBounds().BoxExtent * InstanceTransform.GetScale3D().GetAbs();
		const double Diameter = Extent.GetMax() * 2.0;
		const int32 CullDistance = bOptimizeRendering ? (Diameter < 100.0 ? 8000 : Diameter < 300.0 ? 14000 : 0) : 0;
		UInstancedStaticMeshComponent* Group = nullptr;
		for (UInstancedStaticMeshComponent* Candidate : InstanceGroups)
		{
			if (Candidate->GetStaticMesh() == Element.Mesh && Candidate->OverrideMaterials == Element.Materials
				&& Candidate->InstanceEndCullDistance == CullDistance)
			{
				Group = Candidate;
				break;
			}
		}
		if (!Group)
		{
			Group = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
			Group->SetupAttachment(GetRootComponent());
			Group->SetStaticMesh(Element.Mesh);
			Group->OverrideMaterials = Element.Materials;
			Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Group->SetCanEverAffectNavigation(false);
			if (bOptimizeRendering)
			{
				Group->SetMobility(EComponentMobility::Static);
				Group->SetCullDistances(CullDistance * 3 / 4, CullDistance);
				// Tiny dressing contributes little to the silhouette but many shadow casters.
				if (Diameter < 100.0) Group->SetCastShadow(false);
			}
			InstanceGroups.Add(Group);
		}
		Group->AddInstance(InstanceTransform, false);
	}
}

TArray<UPlanetTerrainClusterVariant*> APlanetGeneratedSector::ResolveVariants() const
{
	TArray<UPlanetTerrainClusterVariant*> Result;
	if (!SectorTemplate || !VariantLibrary) return Result;
	FRandomStream Random(Seed);
	for (const FPlanetSectorClusterSlot& Slot : SectorTemplate->ClusterSlots)
	{
		const TArray<UPlanetTerrainClusterVariant*> Compatible = VariantLibrary->FindCompatible(Slot.Shape);
		Result.Add(Compatible.IsEmpty() ? nullptr : Compatible[Random.RandRange(0, Compatible.Num() - 1)]);
	}
	return Result;
}

const TArray<FBox>& APlanetGeneratedSector::GetPlacementMeshBounds() const
{
	if (bPlacementBoundsReady && BoundsSeed == Seed && BoundsTemplate == SectorTemplate && BoundsLibrary == VariantLibrary)
		return PlacementMeshBounds;
	PlacementMeshBounds.Reset();
	CombinedPlacementBounds = FBox(ForceInit);
	BoundsSeed = Seed;
	BoundsTemplate = SectorTemplate;
	BoundsLibrary = VariantLibrary;
	bPlacementBoundsReady = true;
	const TArray<UPlanetTerrainClusterVariant*> Variants = ResolveVariants();
	for (int32 Index = 0; Index < Variants.Num(); ++Index)
	{
		if (!Variants[Index]) continue;
		for (const FPlanetClusterElement& Element : Variants[Index]->Elements)
		{
			if (Element.Mesh)
			{
				// Match AddComposition, including pivot, slot rotation and nonuniform scale.
				PlacementMeshBounds.Add(Element.Mesh->GetBoundingBox().TransformBy(
					Element.Transform * SectorTemplate->ClusterSlots[Index].Transform));
				CombinedPlacementBounds += PlacementMeshBounds.Last();
			}
		}
	}
	return PlacementMeshBounds;
}

void APlanetGeneratedSector::Generate()
{
	ClearGenerated();
	if (!SectorTemplate || !VariantLibrary)
	{
		Diagnostics.Add(TEXT("Assign SectorTemplate and VariantLibrary."));
		return;
	}
	bPlacementBoundsReady = false;
	GetPlacementMeshBounds();
	const TArray<UPlanetTerrainClusterVariant*> Variants = ResolveVariants();
	for (int32 Index = 0; Index < SectorTemplate->ClusterSlots.Num(); ++Index)
	{
		const FPlanetSectorClusterSlot& Slot = SectorTemplate->ClusterSlots[Index];
		const UPlanetTerrainClusterVariant* Variant = Variants[Index];
		SelectedVariantIds.Add(Variant ? Variant->VariantId : NAME_None);
		if (!Variant)
		{
			Diagnostics.Add(FString::Printf(TEXT("Slot %s: no compatible variants; footprint left empty."), *Slot.SlotId.ToString()));
			continue;
		}
		AddComposition(Variant, Slot.Transform);
	}
	// Upload completed batches once, not one render-state update per instance.
	for (UInstancedStaticMeshComponent* Group : InstanceGroups) Group->RegisterComponent();
	bResident = true;
}

#if WITH_EDITOR
void APlanetTerrainClusterAuthoringActor::BakeClusterVariant()
{
	if (!Shape || !TargetClusterVariant)
	{
		UE_LOG(LogTemp, Error, TEXT("Cluster bake requires Shape and TargetClusterVariant."));
		return;
	}
	TArray<FPlanetClusterElement> Elements;
	TSet<AStaticMeshActor*> Visited;
	for (AStaticMeshActor* Actor : MeshActors)
	{
		if (!IsValid(Actor) || Actor->GetWorld() != GetWorld() || Visited.Contains(Actor)) continue;
		Visited.Add(Actor);
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		if (!Component->GetStaticMesh()) continue;
		FPlanetClusterElement& Element = Elements.AddDefaulted_GetRef();
		Element.Mesh = Component->GetStaticMesh();
		Element.Transform = Component->GetComponentTransform().GetRelativeTransform(GetActorTransform());
		Element.Materials = Component->OverrideMaterials;
	}
	if (Elements.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("No valid MeshActors; existing variant was not overwritten."));
		return;
	}
	TargetClusterVariant->Modify();
	TargetClusterVariant->CompatibleShape = Shape;
	TargetClusterVariant->Elements = MoveTemp(Elements);
	TargetClusterVariant->MarkPackageDirty();
	TargetClusterVariant->PostEditChange();
}
#endif
