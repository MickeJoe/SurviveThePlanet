#include "Gameplay/Save/ColonySaveSubsystem.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Drones/BaseDrone.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/GeothermalSite.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Planet/PlanetWeatherManager.h"
#include "Gameplay/Planet/MissionConfidenceSubsystem.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Work/ConstructionJobQueueSubsystem.h"
#include "Gameplay/Objectives/ObjectiveSubsystem.h"
#include "Gameplay/Contracts/ContractOfferSubsystem.h"
#include "Gameplay/Trading/TradeSubsystem.h"
#include "Gameplay/Cables/CableNetworkManager.h"
#include "Gameplay/Energy/EnergyConnectionComponent.h"
#include "Gameplay/Energy/EnergyCoverageComponent.h"
#include "Components/SceneComponent.h"
#include "Gameplay/UI/ResourceDisplayWidget.h"
#include "SurviveThePlanetPlayerController.h"
#include "SurviveThePlanetCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gameplay/World/SectorPopulation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "TimerManager.h"

namespace ColonySave
{
	using FObjects = TMap<FString, UObject*>;
	constexpr TCHAR Prefix[] = TEXT("Colony_");

	// Actor/component references use snapshot IDs rather than PIE or level package paths.
	class FSnapshotArchive : public FObjectAndNameAsStringProxyArchive
	{
	public:
		FSnapshotArchive(FArchive& Inner, const FObjects& InObjects)
			: FObjectAndNameAsStringProxyArchive(Inner, true), Objects(InObjects) { ArNoDelta = true; }
		using FObjectAndNameAsStringProxyArchive::operator<<;
		virtual FArchive& operator<<(UObject*& Object) override
		{
			FString Key;
			if (IsSaving() && Object)
			{
				if (UBuildingManagerSubsystem::IsRetiredStandaloneBuilding(Cast<ABaseBuilding>(Object))) Key.Empty();
				else if (const FString* Id = Objects.FindKey(Object)) Key = TEXT("@") + *Id;
				else Key = Object->GetPathName();
			}
			InnerArchive << Key;
			if (IsLoading()) Object = Key.StartsWith(TEXT("@")) ? Objects.FindRef(Key.Mid(1))
				: Key.IsEmpty() ? nullptr : StaticLoadObject(UObject::StaticClass(), nullptr, *Key);
			return *this;
		}
		virtual FArchive& operator<<(FObjectPtr& Object) override
		{
			UObject* Value = Object.Get(); *this << Value; if (IsLoading()) Object = Value; return *this;
		}
		virtual FArchive& operator<<(FWeakObjectPtr& Object) override
		{
			UObject* Value = Object.Get(); *this << Value; if (IsLoading()) Object = Value; return *this;
		}
	private:
		const FObjects& Objects;
	};

	bool IsColonyActor(AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) return false;
		if (const auto* Building = Cast<ABaseBuilding>(Actor)) return !Building->IsPlacementPreview();
		return Actor->IsA<ABaseDrone>() || Actor->IsA<ABaseResourceSource>() || Actor->IsA<AGeothermalSite>();
	}
	void AddObject(FObjects& Objects, const FString& Id, UObject* Object)
	{
		Objects.Add(Id, Object);
		if (auto* Actor = Cast<AActor>(Object))
		{
			TArray<UActorComponent*> Components; Actor->GetComponents(Components);
			for (auto* Component : Components) Objects.Add(Id + TEXT("/") + Component->GetName(), Component);
		}
	}
	void CaptureFields(UObject* Object, FColonySavedObject& Record, const FObjects& Objects)
	{
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_SaveGame)) continue;
			auto& Field = Record.Fields.AddDefaulted_GetRef(); Field.Name = It->GetFName();
			FMemoryWriter Writer(Field.Data, true); FSnapshotArchive Archive(Writer, Objects);
			// Select SaveGame fields at the object boundary; nested structs keep all their data.
			It->SerializeItem(FStructuredArchiveFromArchive(Archive).GetSlot(), It->ContainerPtrToValuePtr<void>(Object), nullptr);
		}
	}
	bool RestoreFields(UObject* Object, const FColonySavedObject& Record, const FObjects& Objects, FText* Error = nullptr)
	{
		for (const auto& Field : Record.Fields)
		{
			FProperty* Property = Object->GetClass()->FindPropertyByName(Field.Name);
			if (Field.Name == TEXT("bSettingsResolved") && Object->IsA<UMissionConfidenceSubsystem>()) continue;
			if (!Property || !Property->HasAnyPropertyFlags(CPF_SaveGame))
			{
				if (Error) *Error = FText::FromString(FString::Printf(TEXT("Cannot restore %s: field %s is incompatible."), *Record.Id, *Field.Name.ToString()));
				return false;
			}
			FMemoryReader Reader(Field.Data, true); FSnapshotArchive Archive(Reader, Objects);
			Property->SerializeItem(FStructuredArchiveFromArchive(Archive).GetSlot(), Property->ContainerPtrToValuePtr<void>(Object), nullptr);
			if (Archive.IsError())
			{
				if (Error) *Error = FText::FromString(FString::Printf(TEXT("Cannot restore %s: field %s contains invalid data."), *Record.Id, *Field.Name.ToString()));
				return false;
			}
		}
		return true;
	}
	TArray<UObject*> Systems(UWorld* World)
	{
		TArray<UObject*> Result = {World->GetSubsystem<UObjectiveSubsystem>(), World->GetSubsystem<UMissionConfidenceSubsystem>(),
			World->GetSubsystem<UContractOfferSubsystem>(), World->GetSubsystem<UTradeSubsystem>(),
			World->GetSubsystem<UConstructionJobQueueSubsystem>(), World->GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>()};
		for (TActorIterator<AActor> It(World); It; ++It)
			if (It->IsA<AResourceManager>() || It->IsA<APlanetWeatherManager>() || It->IsA<AHexSectorGrid>() || It->IsA<ACableNetworkManager>()) Result.Add(*It);
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UResourceDisplayWidget::StaticClass(), false);
		for (auto* Widget : Widgets) if (Widget->GetWorld() == World && Widget->IsInViewport()) Result.Add(Widget);
		if (auto* Pawn = UGameplayStatics::GetPlayerPawn(World, 0)) Result.Add(Pawn);
		Result.Remove(nullptr); return Result;
	}
	FString MapName(UWorld* World)
	{
		FString Package = World->GetOutermost()->GetName();
		return Package.Replace(*World->StreamingLevelsPrefix, TEXT(""));
	}
}

void UColonySaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MapLoadedHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UColonySaveSubsystem::MapLoaded);
}
void UColonySaveSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapLoadedHandle);
	Super::Deinitialize();
}
FString UColonySaveSubsystem::GetSuggestedSaveName() const { return FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")); }

bool UColonySaveSubsystem::CanSaveCurrentWorld() const
{
	UWorld* World = GetWorld();
	return World && World->HasBegunPlay() && static_cast<bool>(TActorIterator<AResourceManager>(World));
}

TArray<FColonySaveEntry> UColonySaveSubsystem::ListSaves() const
{
	TArray<FColonySaveEntry> Result;
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(FPaths::ProjectSavedDir() / TEXT("SaveGames/Colony_*.sav")), true, false);
	for (const FString& File : Files)
	{
		const FString Slot = FPaths::GetBaseFilename(File);
		if (auto* Save = Cast<UColonySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
			if (Save->Version == 1 && Save->Entry.Slot == Slot) Result.Add(Save->Entry);
	}
	Result.Sort([](const auto& A, const auto& B) { return A.SavedAt > B.SavedAt; });
	return Result;
}

UColonySaveGame* UColonySaveSubsystem::CaptureWorld(UWorld* World, FText& Status) const
{
	if (!World || !World->HasBegunPlay() || !TActorIterator<AResourceManager>(World)) { Status = FText::FromString(TEXT("No active colony to save.")); return nullptr; }
	auto* Save = NewObject<UColonySaveGame>();
	Save->Entry.Map = ColonySave::MapName(World);
	if (auto* Camera = Cast<ASurviveThePlanetCameraPawn>(UGameplayStatics::GetPlayerPawn(World, 0))) Save->CameraZoom = Camera->GetCameraBoom()->TargetArmLength;
	ColonySave::FObjects Objects;
	for (TActorIterator<AActor> It(World); It; ++It) if (ColonySave::IsColonyActor(*It)
		&& !UBuildingManagerSubsystem::IsRetiredStandaloneBuilding(Cast<ABaseBuilding>(*It)))
	{
		auto& Record = Save->Actors.AddDefaulted_GetRef();
		Record.Id = It->GetName(); Record.Class = FSoftClassPath(It->GetClass()); Record.Transform = It->GetActorTransform(); Record.bTickEnabled = It->IsActorTickEnabled();
		ColonySave::AddObject(Objects, Record.Id, *It);
	}
	for (UObject* System : ColonySave::Systems(World))
	{
		auto& Record = Save->Systems.AddDefaulted_GetRef();
		Record.Id = TEXT("System/") + System->GetClass()->GetPathName(); Record.Class = FSoftClassPath(System->GetClass());
		if (auto* Pawn = Cast<APawn>(System)) Record.Transform = Pawn->GetActorTransform();
		ColonySave::AddObject(Objects, Record.Id, System);
	}
	for (auto& Record : Save->Actors) ColonySave::CaptureFields(Objects.FindChecked(Record.Id), Record, Objects);
	for (auto& Record : Save->Systems) ColonySave::CaptureFields(Objects.FindChecked(Record.Id), Record, Objects);
	for (const auto& Pair : Objects) if (auto* Component = Cast<UActorComponent>(Pair.Value))
	{
		FColonySavedObject Record; Record.Id = Pair.Key; Record.Class = FSoftClassPath(Component->GetClass());
		ColonySave::CaptureFields(Component, Record, Objects);
		if (!Record.Fields.IsEmpty() || Component->IsA<UEnergyCoverageComponent>()) Save->Systems.Add(MoveTemp(Record));
	}
	if (auto* Trade = World->GetSubsystem<UTradeSubsystem>())
	{
		FMemoryWriter Writer(Save->TraderStock); ColonySave::FSnapshotArchive Archive(Writer, Objects); Archive << Trade->TraderStock;
	}
	return Save;
}

bool UColonySaveSubsystem::SaveColony(const FString& DisplayName, FText& Status)
{
	UColonySaveGame* Save = CaptureWorld(GetWorld(), Status); if (!Save) return false;
	Save->Entry.DisplayName = DisplayName.TrimStartAndEnd().Left(80);
	if (Save->Entry.DisplayName.IsEmpty()) Save->Entry.DisplayName = GetSuggestedSaveName();
	Save->Entry.Slot = TEXT("Colony_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Save->Entry.SavedAt = FDateTime::UtcNow();
	const bool Success = UGameplayStatics::SaveGameToSlot(Save, Save->Entry.Slot, 0);
	Status = FText::FromString(Success ? TEXT("Game saved: ") + Save->Entry.DisplayName : TEXT("Save failed. Check free disk space and write permissions."));
	return Success;
}

bool UColonySaveSubsystem::LoadColony(const FString& Slot, FText& Status)
{
	FGuid Id;
	if (!Slot.StartsWith(TEXT("Colony_")) || !FGuid::ParseExact(Slot.Mid(7), EGuidFormats::Digits, Id))
	{ Status = FText::FromString(TEXT("Invalid save slot.")); return false; }
	auto* Save = Cast<UColonySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Save || Save->Version != 1 || Save->Entry.Slot != Slot || !FPackageName::DoesPackageExist(Save->Entry.Map))
	{ Status = FText::FromString(TEXT("This save is damaged, incompatible, or its map is unavailable.")); return false; }
	for (const auto& Record : Save->Actors)
		if (UClass* Class = Record.Class.TryLoadClass<AActor>(); !Class || !Class->IsChildOf(AActor::StaticClass()))
		{ Status = FText::FromString(TEXT("A building or drone required by this save is unavailable.")); return false; }
	PendingLoad = Save;
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	UGameplayStatics::OpenLevel(this, FName(*Save->Entry.Map));
	Status = FText::FromString(TEXT("Loading...")); return true;
}

void UColonySaveSubsystem::MapLoaded(UWorld* World)
{
	if (!PendingLoad || !World || World->GetGameInstance() != GetGameInstance()) return;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UColonySaveSubsystem::FinishLoading, TWeakObjectPtr<UWorld>(World), 0));
}
void UColonySaveSubsystem::FinishLoading(TWeakObjectPtr<UWorld> WeakWorld, int32 Attempts)
{
	UWorld* World = WeakWorld.Get(); if (!World || !PendingLoad) return;
	bool Ready = false;
	for (UObject* System : ColonySave::Systems(World)) Ready |= System->IsA<UResourceDisplayWidget>();
	if (!Ready && Attempts < 120)
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UColonySaveSubsystem::FinishLoading, WeakWorld, Attempts+1)); return;
	}
	FText Status;
	const bool Success = RestoreWorld(World, PendingLoad, Status);
	PendingLoad = nullptr;
	LastLoadStatus = Status;
	UE_LOG(LogTemp, Display, TEXT("Colony load: %s"), *Status.ToString());
	if (auto* PC = Cast<ASurviveThePlanetPlayerController>(World->GetFirstPlayerController())) PC->OpenPauseMenu();
	if (!Success) UE_LOG(LogTemp, Error, TEXT("Colony restore failed: %s"), *Status.ToString());
}

bool UColonySaveSubsystem::RestoreWorld(UWorld* World, UColonySaveGame* Save, FText& Status)
{
	Status = FText::FromString(TEXT("The saved colony could not be completely restored."));
	if (!World || !Save || Save->Version != 1) return false;
	// Remove retired standalone extractors in memory; the on-disk slot is not rewritten.
	// Missing snapshot IDs resolve to null, disconnecting obsolete assignments and jobs.
	Save = DuplicateObject<UColonySaveGame>(Save, this);
	TSet<FString> RemovedActorIds;
	Save->Actors.RemoveAll([&RemovedActorIds](const FColonySavedObject& Record)
	{
		UClass* Class = Record.Class.TryLoadClass<AActor>();
		const auto* Defaults = Class ? Cast<ABaseBuilding>(Class->GetDefaultObject()) : nullptr;
		if (!UBuildingManagerSubsystem::IsRetiredStandaloneBuilding(Defaults)) return false;
		// Both placement routes can use the same Blueprint class. The saved source
		// reference, rather than the class alone, distinguishes a valid deposit mine.
		for (const auto& Field : Record.Fields) if (Field.Name == TEXT("ResourceSource"))
		{
			FMemoryReader Reader(Field.Data, true); FString SourceKey; Reader << SourceKey;
			if (!SourceKey.IsEmpty() && !Reader.IsError()) return false;
		}
		RemovedActorIds.Add(Record.Id); return true;
	});
	Save->Systems.RemoveAll([&RemovedActorIds](const FColonySavedObject& Record)
	{
		FString OwnerId, ComponentName;
		return Record.Id.Split(TEXT("/"), &OwnerId, &ComponentName) && RemovedActorIds.Contains(OwnerId);
	});
	ColonySave::FObjects Objects;
	// Reveal saved sectors before matching actors: population owns their generated deposits.
	for (TActorIterator<AHexSectorGrid> Grid(World); Grid; ++Grid)
	{
		for (const auto& Record : Save->Systems) if (Record.Class.ResolveClass() == Grid->GetClass())
			if (!ColonySave::RestoreFields(*Grid, Record, Objects, &Status)) return false;
		for (const auto& Sector : Grid->Sectors)
			if (Sector.State != ESectorState::Undiscovered) Grid->OnSectorStateChanged.Broadcast(Sector.Id, Sector.State);
	}
	TArray<AActor*> Existing;
	for (TActorIterator<AActor> It(World); It; ++It) if (ColonySave::IsColonyActor(*It)) Existing.Add(*It);
	TSet<AActor*> Used;
	// Match regenerated deposits and authored actors by class/position, never by PIE object names.
	for (const auto& Record : Save->Actors)
	{
		UClass* Class = Record.Class.TryLoadClass<AActor>(); if (!Class) return false;
		AActor* Actor = nullptr;
		for (auto* Candidate : Existing)
			if (!Used.Contains(Candidate) && Candidate->GetClass() == Class && Candidate->GetActorLocation().Equals(Record.Transform.GetLocation(), 1.0f)) { Actor = Candidate; break; }
		if (!Actor)
		{
			FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Actor = World->SpawnActor<AActor>(Class, Record.Transform, Params);
		}
		if (!Actor) { Status = FText::FromString(TEXT("Could not restore a colony actor.")); return false; }
		Used.Add(Actor); Actor->SetActorTransform(Record.Transform);
		ColonySave::AddObject(Objects, Record.Id, Actor);
	}
	for (auto* Actor : Existing) if (!Used.Contains(Actor)) Actor->Destroy();
	// Placed extenders add instance components at runtime. Recreate their logical
	// components before deserializing any references, including legacy version-1 slots.
	for (const auto& Record : Save->Systems)
	{
		if (Objects.Contains(Record.Id) || Record.Id.StartsWith(TEXT("System/"))) continue;
		FString ActorId, ComponentName;
		if (!Record.Id.Split(TEXT("/"), &ActorId, &ComponentName)) continue;
		AActor* Owner = Cast<AActor>(Objects.FindRef(ActorId));
		UClass* ComponentClass = Record.Class.TryLoadClass<UActorComponent>();
		if (!Owner || !ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
		{
			Status = FText::FromString(FString::Printf(TEXT("Cannot recreate saved component %s."), *Record.Id)); return false;
		}
		auto* Component = NewObject<UActorComponent>(Owner, ComponentClass, FName(*ComponentName));
		if (auto* Scene = Cast<USceneComponent>(Component)) Scene->SetupAttachment(Owner->GetRootComponent());
		Owner->AddInstanceComponent(Component);
		Component->RegisterComponent();
		Objects.Add(Record.Id, Component);
	}

	for (UObject* System : ColonySave::Systems(World)) ColonySave::AddObject(Objects, TEXT("System/") + System->GetClass()->GetPathName(), System);
	for (const auto& Record : Save->Actors)
		if (!ColonySave::RestoreFields(Objects.FindChecked(Record.Id), Record, Objects, &Status)) return false;
	// Initialization is a property of this world, not the saved run. Resolve it
	// before restoring confidence so the next tick cannot reset the saved value.
	if (auto* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>()) Mission->ResolvePlanetSettings();
	for (const auto& Record : Save->Systems)
	{
		UObject* System = Objects.FindRef(Record.Id);
		if (!System) { Status = FText::FromString(FString::Printf(TEXT("Cannot find saved system %s."), *Record.Id)); return false; }
		if (!ColonySave::RestoreFields(System, Record, Objects, &Status)) return false;
		if (auto* Pawn = Cast<APawn>(System)) Pawn->SetActorTransform(Record.Transform);
	}
	if (auto* Trade = World->GetSubsystem<UTradeSubsystem>())
	{
		FMemoryReader Reader(Save->TraderStock); ColonySave::FSnapshotArchive Archive(Reader, Objects); Archive << Trade->TraderStock;
	}
	for (TActorIterator<APlanetSurfaceManager> It(World); It; ++It)
	{
		It->OccupiedCells.Reset();
		for (const auto& Pair : Objects)
		{
			AActor* Actor = Cast<AActor>(Pair.Value); if (!Actor) continue;
			FIntPoint Footprint;
			if (auto* Building = Cast<ABaseBuilding>(Actor)) Footprint = Building->GetGridFootprint();
			else if (auto* Source = Cast<ABaseResourceSource>(Actor)) { if (Source->IsReservedForMining()) continue; Footprint = Source->GetGridFootprint(); }
			else continue;
			const auto Placement = It->GetPlacementForWorldLocation(Actor->GetActorLocation(), Footprint);
			for (int32 Y = 0; Y < Footprint.Y; ++Y) for (int32 X = 0; X < Footprint.X; ++X)
				It->OccupiedCells.Add(It->MakeCellKey(FSTPGridCell(Placement.OriginCell.X+X, Placement.OriginCell.Y+Y)), Actor);
		}
		It->InvalidateRoutingObstacles();
	}
	for (const auto& Record : Save->Actors)
	{
		AActor* Actor = CastChecked<AActor>(Objects.FindChecked(Record.Id));
		Actor->SetActorTickEnabled(Record.bTickEnabled);
		if (auto* Drone = Cast<ABaseDrone>(Actor))
		{
			if (!Drone->GetAssignedBuilding() && Drone->GetAssignedBuildingSlot() != INDEX_NONE)
				Drone->SetBuildingAssignmentInternal(nullptr, INDEX_NONE);
			if (Drone->HasOngoingConstructionJob() && !IsValid(Drone->GetConstructionTarget())) Drone->ClearOngoingConstructionJob();
		}
		if (auto* Building = Cast<ABaseBuilding>(Actor)) Building->RefreshConstructionProgressBar();
		if (auto* Source = Cast<ABaseResourceSource>(Actor)) Source->RefreshResourceMeshVisibility();
	}
	for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
	{
		for (auto& Cell : It->CableCells) It->RefreshCableCell(Cell.Key);
		It->RefreshEnergyGrid();
	}
	for (const auto& Pair : Objects) if (auto* Coverage = Cast<UEnergyCoverageComponent>(Pair.Value)) Coverage->RefreshCoverageEligibility();
	for (const auto& Pair : Objects) if (auto* Connection = Cast<UEnergyConnectionComponent>(Pair.Value)) Connection->RestorePlacedRoute();
	for (UObject* System : ColonySave::Systems(World)) if (auto* HUD = Cast<UResourceDisplayWidget>(System))
	{
		HUD->bMenuPaused = false; HUD->bTradingPaused = false;
		HUD->ApplySimulationRate(); HUD->RefreshAllResources(); HUD->RefreshGameTimeDisplay(); HUD->RefreshTimeControlStyles();
	}
	if (auto* Mission = World->GetSubsystem<UMissionConfidenceSubsystem>())
	{
		Mission->OnMissionConfidenceChanged.Broadcast(Mission->GetMissionConfidence(), 0.0f);
		Mission->OnMissionProgressChanged.Broadcast(Mission->GetMissionProgress(), Mission->GetMissionProgressTarget());
	}
	// Rebuild bound widgets from restored state without completing objectives or granting rewards.
	if (auto* Objectives = World->GetSubsystem<UObjectiveSubsystem>())
	{
		Objectives->ReconcileSavedObjectives();
		for (const auto& State : Objectives->RuntimeStates) Objectives->OnObjectiveStateChanged.Broadcast(State.Key, State.Value.State);
	}
	if (auto* Contracts = World->GetSubsystem<UContractOfferSubsystem>()) Contracts->OnContractStateChanged.Broadcast();
	if (auto* Inventory = GetGameInstance()->GetSubsystem<UBuildingBlueprintSubsystem>())
		for (auto Tool : Inventory->GetOwnedBlueprints()) Inventory->OnInventoryChanged.Broadcast(Tool);
	if (auto* Camera = Cast<ASurviveThePlanetCameraPawn>(UGameplayStatics::GetPlayerPawn(World, 0)))
		if (Save->CameraZoom > 0.0f) Camera->GetCameraBoom()->TargetArmLength = Save->CameraZoom;
	Status = FText::FromString(TEXT("Game loaded.")); return true;
}
