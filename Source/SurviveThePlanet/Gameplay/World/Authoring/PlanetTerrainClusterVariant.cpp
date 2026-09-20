#include "PlanetTerrainClusterVariant.h"
#include "PlanetSectorTemplate.h"
#include "PlanetTerrainClusterShape.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

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
}

void APlanetGeneratedSector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Generate();
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
}

void APlanetGeneratedSector::AddComposition(const UPlanetTerrainClusterVariant* Variant, const FTransform& SlotTransform)
{
	for (const FPlanetClusterElement& Element : Variant->Elements)
	{
		if (!Element.Mesh) continue;
		UInstancedStaticMeshComponent* Group = nullptr;
		for (UInstancedStaticMeshComponent* Candidate : InstanceGroups)
		{
			if (Candidate->GetStaticMesh() == Element.Mesh && Candidate->OverrideMaterials == Element.Materials)
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
			Group->RegisterComponent();
			InstanceGroups.Add(Group);
		}
		Group->AddInstance(Element.Transform * SlotTransform, false);
	}
}

void APlanetGeneratedSector::Generate()
{
	ClearGenerated();
	if (!SectorTemplate || !VariantLibrary)
	{
		Diagnostics.Add(TEXT("Assign SectorTemplate and VariantLibrary."));
		return;
	}
	FRandomStream Random(Seed);
	for (const FPlanetSectorClusterSlot& Slot : SectorTemplate->ClusterSlots)
	{
		const TArray<UPlanetTerrainClusterVariant*> Compatible = VariantLibrary->FindCompatible(Slot.Shape);
		if (Compatible.IsEmpty())
		{
			SelectedVariantIds.Add(NAME_None);
			Diagnostics.Add(FString::Printf(TEXT("Slot %s: no compatible variants; footprint left empty."), *Slot.SlotId.ToString()));
			continue;
		}
		const UPlanetTerrainClusterVariant* Variant = Compatible[Random.RandRange(0, Compatible.Num() - 1)];
		SelectedVariantIds.Add(Variant->VariantId);
		AddComposition(Variant, Slot.Transform);
	}
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
