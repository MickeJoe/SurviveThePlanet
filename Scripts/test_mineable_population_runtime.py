"""Run with -ExecCmds="py <path>" in a dedicated editor; validates actual PIE BeginPlay."""
import unreal, time, json
from pathlib import Path
project = Path(unreal.Paths.project_dir()).resolve()
assert project == Path(r"C:\UE5\SurviveThePlanet 5.8")
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.EditorLoadingAndSavingUtils.load_map("/Game/WorldGeneration/Maps/L_PlanetClusters")
started = time.monotonic()

def check(delta):
    if time.monotonic() - started < 12:
        return
    result = {}
    try:
        world = editor.get_game_world()
        assert world, "PIE world did not start"
        population = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SectorPopulation)[0]
        surface = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlanetSurfaceManager)[0]
        grid = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HexSectorGrid)[0]
        assert not population.get_editor_property("diagnostics"), str(population.get_editor_property("diagnostics"))
        for sector in grid.get_editor_property("sectors"):
            population.reveal_sector(sector.id)
        sources = list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.BaseResourceSource))
        checked = 0
        statics = unreal.get_default_object(unreal.GameplayStatics)
        def spawn_deferred(mine_class, transform):
            return statics.call_method("BeginDeferredActorSpawnFromClass", (world, mine_class, transform,
                unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        def finish_spawn(actor, transform):
            return statics.call_method("FinishSpawningActor", (actor, transform, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        for source in sources:
            defaults = unreal.get_default_object(source.get_mine_blueprint())
            transform = defaults.get_placement_transform_for_source(source)
            preview = spawn_deferred(source.get_mine_blueprint(), transform)
            preview.set_placement_preview(True)
            finish_spawn(preview, transform)
            assert preview.can_mine_resource_source(source), f"Runtime preview failed: {source.get_name()}"
            mine = spawn_deferred(source.get_mine_blueprint(), transform)
            finish_spawn(mine, transform)
            mine.set_placement_preview(False)
            assert mine.attach_to_resource_source(source), f"Runtime attach failed: {source.get_name()}"
            assert source.get_reserved_mining_machine() == mine
            preview.destroy_actor()
            checked += 1
        result = {"passed": True, "runtime_deposits": checked, "sectors": len(grid.get_editor_property("sectors"))}
        unreal.log("STP_RUNTIME_MINES PASSED " + json.dumps(result))
    except Exception as error:
        result = {"passed": False, "error": str(error)}
        unreal.log_error("STP_RUNTIME_MINES FAILED " + repr(error))
    finally:
        (project / "Saved" / "MineablePopulationRuntime.json").write_text(json.dumps(result, indent=2))
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()

handle = unreal.register_slate_post_tick_callback(check)
levels.editor_request_begin_play()
