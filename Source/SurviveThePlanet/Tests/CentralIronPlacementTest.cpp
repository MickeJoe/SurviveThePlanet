#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "SurviveThePlanetPlayerController.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/MiningMachine.h"
#include "Gameplay/Resources/BaseResourceSource.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/World/HexSectorGrid.h"
#include "Gameplay/Cables/CableNetworkManager.h"

class FCentralIronPlacementCommand : public IAutomationLatentCommand
{
public:
    explicit FCentralIronPlacementCommand(FAutomationTestBase* InTest) : Test(InTest), Started(FPlatformTime::Seconds()) {}
    virtual bool Update() override
    {
        UWorld* World = nullptr;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
            if (Context.WorldType == EWorldType::PIE) { World = Context.World(); break; }
        if (FPlatformTime::Seconds() - Started > 30) { Test->AddError(TEXT("Central iron placement timed out")); return true; }
        if (!World) return false;
        auto* PC = Cast<ASurviveThePlanetPlayerController>(World->GetFirstPlayerController());
        if (!PC || !PC->GetPawn()) return false;
        if (Stage == 0)
        {
            AHexSectorGrid* Grid = nullptr;
            for (TActorIterator<AHexSectorGrid> It(World); It; ++It) { Grid = *It; break; }
            if (!Grid) return false;
            for (TActorIterator<ABaseResourceSource> It(World); It; ++It)
                if (It->GetResourceType() == EResourceType::Iron
                    && Grid->GetSectorAtWorldLocation(It->GetActorLocation()) == Grid->StartingSectorId) { Source = *It; break; }
            if (!Source.IsValid()) return false;
            ABaseBuilding* HQ = nullptr;
            for (TActorIterator<ABaseBuilding> It(World); It; ++It)
                if (It->GetBuildingType() == ESTPBuildingType::BaseModule) { HQ = *It; break; }
            if (!Test->TestNotNull(TEXT("Starting HQ"), HQ)) return true;
            const AMiningMachine* Defaults = Source->GetMineBlueprint()->GetDefaultObject<AMiningMachine>();
            const float Distance = FVector::Dist2D(HQ->GetActorLocation(), Defaults->GetPlacementTransformForSource(Source.Get()).GetLocation());
            Test->TestTrue(TEXT("Iron mine is within 1600 cm of HQ"), Distance <= 1600.0f);
            for (const FHexSector& Sector : Grid->Sectors)
                if (Sector.Id != Grid->StartingSectorId) Test->TestTrue(TEXT("Neighbour remains undiscovered"), Sector.State == ESectorState::Undiscovered);
            for (TActorIterator<ABaseBuilding> It(World); It; ++It)
                Test->TestFalse(TEXT("No extender was needed"), It->GetBuildingData() && It->GetBuildingData()->BuildTool == ESTPBuildTool::EnergyExtender);
            PC->EdgeScrollZone = 0;
            PC->GetPawn()->SetActorLocation(Source->GetActorLocation());
            UWidgetBlueprintLibrary::SetFocusToGameViewport();
            Stage = 1; Frames = 0; return false;
        }
        if (++Frames < 3) return false;
        if (Stage == 1)
        {
            FVector2D Screen;
            if (!Test->TestTrue(TEXT("Iron projects into viewport"), PC->ProjectWorldLocationToScreen(Source->GetComponentsBoundingBox(true).GetCenter(), Screen))) return true;
            PC->SetMouseLocation(FMath::RoundToInt(Screen.X), FMath::RoundToInt(Screen.Y));
            Stage = 2; Frames = 0; return false;
        }
        FHitResult Hit;
        PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit);
        if (!Test->TestEqual(TEXT("Real cursor trace hits iron"), Hit.GetActor(), static_cast<AActor*>(Source.Get()))) return true;
        PC->SetActiveBuildTool(ESTPBuildTool::MiningMachine);
        PC->UpdateMiningMachinePlacementPreview();
        AMiningMachine* Preview = PC->MiningMachinePlacementPreview;
        if (!Test->TestNotNull(TEXT("Real mining preview"), Preview)) return true;
        if (!Test->TestTrue(TEXT("Real preview is valid with starting resources and locked neighbours"), Preview->IsMiningPlacementPreviewValid())) return true;
        AResourceManager* Resources = nullptr;
        for (TActorIterator<AResourceManager> It(World); It; ++It) { Resources = *It; break; }
        if (!Test->TestNotNull(TEXT("Starting resources"), Resources)) return true;
        Test->TestEqual(TEXT("Untouched 20 starting iron"), Resources->GetResourceAmount(EResourceType::Iron), 20);
        PC->TryPlaceMiningMachineAtCursor();
        auto* Mine = Cast<AMiningMachine>(PC->GetSelectedActor());
        if (!Test->TestNotNull(TEXT("Controller click creates the mine"), Mine)) return true;
        Test->TestEqual(TEXT("Mine attaches to starting iron"), Mine->GetResourceSource(), Source.Get());
        Test->TestEqual(TEXT("Click spends 10 iron"), Resources->GetResourceAmount(EResourceType::Iron), 10);
        Mine->SetConstructionProgress(1.0f);
        for (TActorIterator<ACableNetworkManager> It(World); It; ++It)
        {
            It->RefreshEnergyGrid();
            Test->TestTrue(TEXT("Finished mine has HQ power without an extender"), It->IsBuildingConnectedToPowerGrid(Mine));
            break;
        }
        return true;
    }
private:
    FAutomationTestBase* Test;
    TWeakObjectPtr<ABaseResourceSource> Source;
    double Started;
    int32 Stage = 0;
    int32 Frames = 0;
};

static bool RunCentralIronTest(FAutomationTestBase* Test, const TCHAR* Map)
{
    FAutomationEditorCommonUtils::LoadMap(Map);
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
    ADD_LATENT_AUTOMATION_COMMAND(FCentralIronPlacementCommand(Test));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCentralIronNexarisTest, "SurviveThePlanet.Start.CentralIron.Nexaris", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCentralIronNexarisTest::RunTest(const FString& Parameters) { return RunCentralIronTest(this, TEXT("/Game/Planet/Maps/L_Nexaris")); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCentralIronAridusTest, "SurviveThePlanet.Start.CentralIron.Aridus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCentralIronAridusTest::RunTest(const FString& Parameters) { return RunCentralIronTest(this, TEXT("/Game/Planet/Maps/L_Aridus")); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCentralIronBorealisTest, "SurviveThePlanet.Start.CentralIron.Borealis", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCentralIronBorealisTest::RunTest(const FString& Parameters) { return RunCentralIronTest(this, TEXT("/Game/Planet/Maps/L_Borealis")); }
#endif
