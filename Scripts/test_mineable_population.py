"""Use -ExecutePythonScript in a dedicated editor. Production-map seed regression; saves no assets."""
import unreal, json, time
from collections import Counter
from pathlib import Path

project = Path(unreal.Paths.project_dir()).resolve()
assert project == Path(r"C:\UE5\SurviveThePlanet 5.8")
report = {"passed": False, "seeds": [], "deposits": 0, "sectors": 0}
report_path = project / "Saved" / "MineablePopulationReport.json"

def run_validation():
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/WorldGeneration/Maps/L_PlanetClusters")
    world = editor.get_editor_world()
    find = lambda cls: list(unreal.GameplayStatics.get_all_actors_of_class(world, cls))
    statics = unreal.get_default_object(unreal.GameplayStatics)
    def deferred(cls, transform):
        return statics.call_method("BeginDeferredActorSpawnFromClass", (world, cls, transform,
            unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    def finish(actor, transform):
        return statics.call_method("FinishSpawningActor", (actor, transform, unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))

    surface = find(unreal.PlanetSurfaceManager)[0]
    grid = find(unreal.HexSectorGrid)[0]
    grid.rebuild_grid()
    assert find(unreal.BaseModuleSpawnPoint)[0].spawn_base_module(), "Base Module could not spawn"
    template = find(unreal.SectorPopulation)[0]
    config = {name: template.get_editor_property(name) for name in (
        "authored_sector_template", "authored_sector_templates", "cluster_variant_library",
        "deposit_classes", "resource_terrain_margin_cells", "manage_cluster_residency", "optimize_cluster_rendering")}
    population_class = template.get_class()
    template.destroy_actor()
    started = time.monotonic()
    for seed in [10, 71237, 1337] + [s for s in range(32) if s != 10]:
        report["current_seed"] = seed
        report_path.write_text(json.dumps(report, indent=2))
        transform = unreal.Transform()
        population = deferred(population_class, transform)
        for name, value in config.items():
            population.set_editor_property(name, value)
        population.set_editor_property("grid", grid)
        population.set_editor_property("seed", seed)
        finish(population, transform)
        population.initialize_population()
        assert not list(population.get_editor_property("diagnostics")), str(population.get_editor_property("diagnostics"))
        deposits = list(population.get_editor_property("resources").deposits)
        sectors = list(grid.get_editor_property("sectors"))
        counts = Counter(d.sector_id for d in deposits)
        start_id = grid.get_editor_property("starting_sector_id")
        start_types = Counter(d.resource_type for d in deposits if d.sector_id == start_id)
        assert start_types == Counter({unreal.ResourceType.STONE: 1, unreal.ResourceType.COPPER: 1, unreal.ResourceType.COAL: 1}), str(start_types)
        assert all(1 <= counts[s.id] <= 3 for s in sectors)
        for sector in sectors:
            population.reveal_sector(sector.id)
        sources = find(unreal.BaseResourceSource)
        assert len(sources) == len(deposits), (len(sources), len(deposits))
        for source in sources:
            assert surface.reserve_cells(source, source.get_grid_cell(), source.get_grid_footprint())
        for sector in find(unreal.PlanetGeneratedSector):
            sector.clear_generated()
        mines = []
        for source in sources:
            mine_class = source.get_mine_blueprint()
            defaults = unreal.get_default_object(mine_class)
            transform = defaults.get_placement_transform_for_source(source)
            predicted = defaults.get_placement_transform_for_source_at_transform(
                unreal.get_default_object(source.get_class()), source.get_actor_transform())
            assert (predicted.translation - transform.translation).length() < 0.01, "CDO/source alignment differs"
            preview = deferred(mine_class, transform)
            preview.set_placement_preview(True)
            finish(preview, transform)
            assert preview.get_grid_footprint() == defaults.get_grid_footprint(), "CDO/preview footprint differs"
            assert preview.can_mine_resource_source(source), f"Invalid preview: seed={seed} source={source.get_name()}"
            placement = surface.get_placement_for_world_location(transform.translation, preview.get_grid_footprint())
            margin = surface.get_building_clearance_cells() + population.get_editor_property("resource_terrain_margin_cells")
            assert surface.has_terrain_clearance(placement.origin_cell, preview.get_grid_footprint(), margin, True)
            mine = deferred(mine_class, transform)
            finish(mine, transform)
            mine.set_placement_preview(False)
            assert mine.attach_to_resource_source(source), f"Attach failed: seed={seed} source={source.get_name()}"
            assert source.get_reserved_mining_machine() == mine
            mines.append(mine)
            preview.destroy_actor()
        for i, first in enumerate(deposits):
            for second in deposits[i+1:]:
                minimum = 1800 if first.sector_id == start_id and second.sector_id == start_id else 1200
                assert (first.location - second.location).length() >= minimum
        report["seeds"].append(seed)
        report["deposits"] += len(deposits)
        report["sectors"] += len(sectors)
        unreal.log(f"STP_RESOURCE_TEST seed={seed} sectors={len(sectors)} deposits={len(deposits)} PASSED")
        # Editor fixtures have no EndPlay; release every reservation explicitly.
        for source, mine in zip(sources, mines):
            surface.release_cells(mine)
            source.release_mining_machine(mine)
            mine.destroy_actor()
            source.destroy_actor()
        for sector in find(unreal.PlanetGeneratedSector):
            sector.destroy_actor()
        population.destroy_actor()
    report["elapsed_seconds"] = time.monotonic() - started
    report["passed"] = True
    unreal.log("STP_RESOURCE_TEST ALL PASSED " + json.dumps(report))

try:
    run_validation()
except Exception as error:
    report["error"] = str(error)
    unreal.log_error("STP_RESOURCE_TEST FAILED " + repr(error))
finally:
    report_path.write_text(json.dumps(report, indent=2))
    unreal.SystemLibrary.quit_editor()
