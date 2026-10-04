#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Components/CanvasPanelSlot.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Gameplay/UI/BuildToolbarWidget.h"
#include "Gameplay/Base/BuildingDataAsset.h"
#include "Gameplay/Base/BaseBuilding.h"
#include "Gameplay/Buildings/BuildingBlueprintSubsystem.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"
#include "SurviveThePlanetPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSTPBlueprintToolbarTest,
    "SurviveThePlanet.UI.BlueprintToolbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSTPBlueprintToolbarTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    GI->InitializeStandalone();
    UWorld* World = GI->GetWorld();
    World->InitializeActorsForPlay(FURL());
    UBuildingManagerSubsystem* Manager = World->GetSubsystem<UBuildingManagerSubsystem>();
    UBuildingBlueprintSubsystem* Inventory = GI->GetSubsystem<UBuildingBlueprintSubsystem>();
    ASurviveThePlanetPlayerController* Controller = World->SpawnActor<ASurviveThePlanetPlayerController>();
    UGameViewportClient* Viewport = NewObject<UGameViewportClient>(GEngine);
    GI->GetWorldContext()->GameViewport = Viewport;
    Viewport->Init(*GI->GetWorldContext(), GI, false);
    FString PlayerError;
    ULocalPlayer* LocalPlayer = GI->CreateLocalPlayer(0, PlayerError, false);
    Controller->SetPlayer(LocalPlayer);
    UClass* ToolbarClass = LoadClass<UBuildToolbarWidget>(nullptr, TEXT("/Game/UI/WBP_BuildToolbar.WBP_BuildToolbar_C"));
    UBuildToolbarWidget* Toolbar = CreateWidget<UBuildToolbarWidget>(Controller, ToolbarClass);
    if (TestNotNull(TEXT("Authored toolbar"), Toolbar) && Manager && Inventory)
    {
        TSharedRef<SWidget> SlateWidget = Toolbar->TakeWidget();
        TestEqual(TEXT("Widget world"), Toolbar->GetWorld(), World);
        TestEqual(TEXT("Widget player"), Toolbar->GetOwningPlayer(), static_cast<APlayerController*>(Controller));
        for (UWidget* Ancestor = Toolbar->DesignedHost; Ancestor; Ancestor = Ancestor->GetParent())
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Ancestor->Slot))
                AddInfo(FString::Printf(TEXT("Toolbar canvas %s autosize=%d size=%s alignment=%s"),
                    *Ancestor->GetName(), CanvasSlot->GetAutoSize(), *CanvasSlot->GetSize().ToString(), *CanvasSlot->GetAlignment().ToString()));
        }

        TestNotNull(TEXT("Authored horizontal host preserved"), Toolbar->DesignedHost.Get());
        TestEqual(TEXT("All four categories exist"), Toolbar->CategoryRows.Num(), 4);
        UBuildingDataAsset* Definition = Manager->GetDefinition(ESTPBuildTool::GeothermalPlant);
        if (TestNotNull(TEXT("Geothermal catalog definition"), Definition))
        {
            AddInfo(FString::Printf(TEXT("Geothermal initially owned: %d"), Definition->bBlueprintInitiallyOwned));
            TestNotNull(TEXT("Geothermal icon"), Definition->ToolbarIcon ? Definition->ToolbarIcon.Get() : Definition->Thumbnail.Get());
            TSubclassOf<ABaseBuilding> BuildingClass = Manager->GetBuildingClass(Definition->BuildTool);
            TestNotNull(TEXT("Geothermal placement class"), BuildingClass.Get());
            const bool InitiallyOwned = Definition->bBlueprintInitiallyOwned;
            Definition->bBlueprintInitiallyOwned = false;
            Inventory->RevokeBlueprint(Definition->BuildTool);
            Toolbar->RefreshToolbarVisibility();
            UWidget* ToolWidget = Toolbar->ToolWidgets.FindRef(Definition->BuildTool);
            UButton* Button = Toolbar->ToolButtons.FindRef(Definition->BuildTool);
            if (TestNotNull(TEXT("Missing designed button supplied dynamically"), Button) &&
                TestNotNull(TEXT("Dynamic widget registered"), ToolWidget))
            {
                TestTrue(TEXT("Unowned blueprint hidden"), ToolWidget->GetVisibility() == ESlateVisibility::Collapsed);
                Toolbar->ActiveCategory = ESTPBuildCategory::Industry;
                TestTrue(TEXT("Grant by ID succeeds"), Inventory->GrantBlueprintById(TEXT("GeothermalPlant")));
                TestTrue(TEXT("Grant reveals icon immediately"), ToolWidget->GetVisibility() == ESlateVisibility::Visible);
                TestTrue(TEXT("Grant selects energy category"), Toolbar->ActiveCategory == ESTPBuildCategory::Energy);
                if (!Definition->ConstructionCosts.IsEmpty())
                    TestFalse(TEXT("Owned but unaffordable button disabled"), Button->GetIsEnabled());
                AResourceManager* Resources = World->SpawnActor<AResourceManager>();
                World->GetTimerManager().Tick(0.3f);
                for (const FResourceCost& Cost : Definition->ConstructionCosts)
                    Resources->AddResource(Cost.Resource, Cost.Cost);

                TestTrue(TEXT("Affordable owned button enabled"), Button->GetIsEnabled());
                Button->OnClicked.Broadcast();
                TestTrue(TEXT("Click selects geothermal placement"), Controller->GetActiveBuildTool() == ESTPBuildTool::GeothermalPlant);
                Controller->SetActiveBuildTool(ESTPBuildTool::None);
                if (BuildingClass)
                {
                    ABaseBuilding* Building = World->SpawnActor<ABaseBuilding>(BuildingClass);
                    TestNotNull(TEXT("Resolved placement class can spawn"), Building);
                    if (Building) Building->Destroy();
                }

                const int32 ButtonCount = Toolbar->ToolButtons.Num();
                Toolbar->BindDesignedToolbar();
                TestEqual(TEXT("Rebinding creates no duplicates"), Toolbar->ToolButtons.Num(), ButtonCount);
            }
            Definition->bBlueprintInitiallyOwned = InitiallyOwned;
        }
        UButton* ExtenderButton = Toolbar->ToolButtons.FindRef(ESTPBuildTool::EnergyExtender);
        UWidget* ExtenderWidget = Toolbar->ToolWidgets.FindRef(ESTPBuildTool::EnergyExtender);
        if (TestNotNull(TEXT("Energy Extender button supplied by catalog"), ExtenderButton) &&
            TestNotNull(TEXT("Energy Extender widget"), ExtenderWidget))
        {
            Toolbar->ActiveCategory = ESTPBuildCategory::Energy;
            Toolbar->RefreshToolbarVisibility();
            TestTrue(TEXT("Extender visible under ENERGY"), ExtenderWidget->GetVisibility() == ESlateVisibility::Visible);
            UBuildingDataAsset* ExtenderDefinition = Manager->GetDefinition(ESTPBuildTool::EnergyExtender);
            if (ExtenderDefinition && TestNotNull(TEXT("Toolbar resource manager"), Toolbar->ResourceManager.Get()))
            {
                for (const FResourceCost& Cost : ExtenderDefinition->ConstructionCosts)
                    Toolbar->ResourceManager->AddResource(Cost.Resource, Cost.Cost);
            }
            Toolbar->RefreshButtonStates();
            TestTrue(TEXT("Affordable extender button enabled"), ExtenderButton->GetIsEnabled());
            ExtenderButton->OnClicked.Broadcast();
            TestEqual(TEXT("Extender button selects its placement tool"), Controller->GetActiveBuildTool(), ESTPBuildTool::EnergyExtender);
            Controller->SetActiveBuildTool(ESTPBuildTool::None);
        }
        for (UBuildingDataAsset* Entry : Manager->GetToolbarDefinitions())
        {
            AddInfo(FString::Printf(TEXT("Catalog %s: class=%s icon=%s"), *Entry->GetName(),
                *GetNameSafe(Manager->GetBuildingClass(Entry->BuildTool).Get()),
                *GetNameSafe(Entry->ToolbarIcon ? Entry->ToolbarIcon.Get() : Entry->Thumbnail.Get())));
        }
        Toolbar->NativeDestruct();
    }
    GI->Shutdown();
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
