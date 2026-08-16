#include "Gameplay/Buildings/BuildingManagerSubsystem.h"

#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Buildings/BuildingCatalogDataAsset.h"
#include "Gameplay/Buildings/ConcretePlant.h"
#include "Gameplay/Buildings/CommunicationModule.h"
#include "Gameplay/Buildings/EnergyModule.h"
#include "Gameplay/Buildings/EnergyStorageBuilding.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Buildings/WaterCollector.h"

void UBuildingManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadDefaultCatalog();
}

void UBuildingManagerSubsystem::LoadDefaultCatalog()
{
	Catalog = LoadObject<UBuildingCatalogDataAsset>(nullptr, TEXT("/Game/Data/Buildings/DA_BuildingCatalog.DA_BuildingCatalog"));
	RebuildIndex();
	if (!Catalog)
	{
		struct FFallbackDefinition { ESTPBuildTool Tool; const TCHAR* Path; };
		static const FFallbackDefinition Fallbacks[] = {
			{ESTPBuildTool::EnergyModule, TEXT("/Game/Data/Buildings/DA_EnergyModule.DA_EnergyModule")},
			{ESTPBuildTool::EnergyStorage, TEXT("/Game/Data/Buildings/DA_EnergyBatteryStorage.DA_EnergyBatteryStorage")},
			{ESTPBuildTool::MiningMachine, TEXT("/Game/Data/Buildings/DA_MiningMachine.DA_MiningMachine")},
			{ESTPBuildTool::WaterCollector, TEXT("/Game/Data/Buildings/DA_WaterCollector.DA_WaterCollector")},
			{ESTPBuildTool::ConcretePlant, TEXT("/Game/Data/Buildings/DA_ConcretePlant.DA_ConcretePlant")}
		};
		for (const FFallbackDefinition& Entry : Fallbacks)
		{
			if (UBuildingDataAsset* Definition = LoadObject<UBuildingDataAsset>(nullptr, Entry.Path))
				DefinitionsByTool.Add(Entry.Tool, Definition);
		}
	}
}

void UBuildingManagerSubsystem::SetCatalog(UBuildingCatalogDataAsset* NewCatalog)
{
	if (Catalog == NewCatalog) return;
	Catalog = NewCatalog;
	RebuildIndex();
	OnCatalogChanged.Broadcast(Catalog);
}

void UBuildingManagerSubsystem::RebuildIndex()
{
	DefinitionsByTool.Reset();
	if (!Catalog) return;
	for (UBuildingDataAsset* Definition : Catalog->Buildings)
	{
		if (IsValid(Definition) && Definition->BuildTool != ESTPBuildTool::None)
		{
			DefinitionsByTool.Add(Definition->BuildTool, Definition);
		}
	}
}

UBuildingDataAsset* UBuildingManagerSubsystem::GetDefinition(ESTPBuildTool Tool) const
{
	if (const TObjectPtr<UBuildingDataAsset>* Found = DefinitionsByTool.Find(Tool)) return Found->Get();
	return nullptr;
}

TSubclassOf<ABaseBuilding> UBuildingManagerSubsystem::GetBuildingClass(ESTPBuildTool Tool) const
{
	if (const TSubclassOf<ABaseBuilding>* Override = RuntimeClassOverrides.Find(Tool)) return *Override;
	if (const UBuildingDataAsset* Definition = GetDefinition(Tool))
	{
		// Building blueprints load their own data assets in their constructors. Keeping
		// this reference soft prevents the catalog -> data asset -> blueprint -> data
		// asset load cycle that can otherwise deadlock AsyncLoading2 during startup.
		if (!Definition->BuildingClass.IsNull())
		{
			if (UClass* LoadedClass = Definition->BuildingClass.LoadSynchronous())
			{
				return LoadedClass;
			}
		}
	}

	switch (Tool)
	{
	case ESTPBuildTool::ConcretePlant:
		if (UClass* BPClass = LoadClass<AConcretePlant>(nullptr, TEXT("/Game/BluePrints/ConcretePlant/BP_ConcretePlant.BP_ConcretePlant_C"))) return BPClass;
		return AConcretePlant::StaticClass();
	case ESTPBuildTool::CommunicationModule: return ACommunicationModule::StaticClass();
	case ESTPBuildTool::WaterCollector: return AWaterCollector::StaticClass();
	case ESTPBuildTool::MiningMachine: return AMiningMachine::StaticClass();
	case ESTPBuildTool::EnergyStorage: return AEnergyStorageBuilding::StaticClass();
	case ESTPBuildTool::EnergyModule: return AEnergyModule::StaticClass();
	default: return nullptr;
	}
}

void UBuildingManagerSubsystem::SetBuildingClassOverride(ESTPBuildTool Tool, TSubclassOf<ABaseBuilding> BuildingClass)
{
	if (BuildingClass) RuntimeClassOverrides.Add(Tool, BuildingClass);
	else RuntimeClassOverrides.Remove(Tool);
}

TArray<UBuildingDataAsset*> UBuildingManagerSubsystem::GetToolbarDefinitions() const
{
	TArray<UBuildingDataAsset*> Result;
	if (Catalog)
	{
		for (UBuildingDataAsset* Definition : Catalog->Buildings)
		{
			if (IsValid(Definition) && Definition->bShowInBuildToolbar) Result.Add(Definition);
		}
	}
	else
	{
		for (const TPair<ESTPBuildTool, TObjectPtr<UBuildingDataAsset>>& Pair : DefinitionsByTool)
			if (IsValid(Pair.Value) && Pair.Value->bShowInBuildToolbar) Result.Add(Pair.Value);
	}
	Result.Sort([](const UBuildingDataAsset& A, const UBuildingDataAsset& B) { return A.ToolbarSortOrder < B.ToolbarSortOrder; });
	return Result;
}
