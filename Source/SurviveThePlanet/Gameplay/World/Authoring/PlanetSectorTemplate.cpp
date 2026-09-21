#include "PlanetSectorTemplate.h"

UPlanetSectorTemplate* UPlanetSectorTemplate::SelectForSector(const TArray<UPlanetSectorTemplate*>& Templates,
	int32 WorldSeed, int32 SectorId, bool bStartingSector)
{
	TArray<UPlanetSectorTemplate*> Eligible;
	for (UPlanetSectorTemplate* Template : Templates)
	{
		if (Template && (!bStartingSector || Template->bCanBeStartingSector)) Eligible.AddUnique(Template);
	}
	if (Eligible.IsEmpty()) return nullptr;
	Eligible.Sort([](const UPlanetSectorTemplate& A, const UPlanetSectorTemplate& B)
	{
		return A.GetPathName() < B.GetPathName();
	});
	const uint32 SelectionSeed = static_cast<uint32>(WorldSeed)
		^ (static_cast<uint32>(SectorId) * 7919u) ^ 0x53454354u;
	FRandomStream Random(static_cast<int32>(SelectionSeed & 0x7fffffffu));
	return Eligible[Random.RandRange(0, Eligible.Num() - 1)];
}
