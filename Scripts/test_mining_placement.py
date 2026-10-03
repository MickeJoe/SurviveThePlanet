"""Isolated commandlet regression: preview -> spawn -> attach for real mining Blueprints.
Run with UnrealEditor-Cmd <project> -run=pythonscript -script=<this file> -unattended -nullrhi.
Creates an unsaved blank world; never run inside an editor containing unsaved work.
"""
import unreal
from pathlib import Path

assert Path(unreal.Paths.project_dir()).resolve() == Path(r"C:\UE5\SurviveThePlanet 5.8")
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
surface = actors.spawn_actor_from_class(unreal.PlanetSurfaceManager, unreal.Vector(0, 0, 0))

for resource_name, mine_path in [
    ("BP_StoneSource", "/Game/BluePrints/StoneQuery/BP_StoneQuerry.BP_StoneQuerry_C"),
    ("BP_CopparSource", "/Game/BluePrints/CopparMiningQuerry/BP_CopparMiningQuerry.BP_CopparMiningQuerry_C"),
    ("BP_IronSource", "/Game/BluePrints/IronMiningQuerry/BP_MiningQuerry.BP_MiningQuerry_C"),
]:
    source_class = unreal.load_class(None, f"/Game/BluePrints/Resources/{resource_name}.{resource_name}_C")
    mine_class = unreal.load_class(None, mine_path)
    assert source_class and mine_class
    source = actors.spawn_actor_from_class(source_class, unreal.Vector(1500, 1500, 0))
    assert source.get_remaining_amount() > 0
    assert surface.reserve_cells(source, source.get_grid_cell(), source.get_grid_footprint())
    preview = actors.spawn_actor_from_class(mine_class, unreal.Vector(1500, 1500, 0))
    preview.set_placement_preview(True)
    transform = preview.get_placement_transform_for_source(source)
    preview.set_actor_transform(transform, False, False)
    assert preview.can_mine_resource_source(source), "Preview must accept the resource"

    # Another real building still blocks clearance.
    blocker = actors.spawn_actor_from_class(mine_class, transform.translation, transform.rotation.rotator())
    blocker.set_actor_transform(transform, False, False)
    blocker.set_placement_preview(False)
    assert not preview.can_mine_resource_source(source), "Other buildings must remain blockers"
    actors.destroy_actor(blocker)
    assert preview.can_mine_resource_source(source)

    mine = actors.spawn_actor_from_class(mine_class, transform.translation, transform.rotation.rotator())
    mine.set_actor_transform(transform, False, False)
    mine.set_placement_preview(False)
    assert mine.can_mine_resource_source(source), "Spawned mine must ignore itself"
    assert mine.attach_to_resource_source(source), "Valid preview must attach after spawn"
    assert source.get_reserved_mining_machine() == mine
    assert not preview.can_mine_resource_source(source), "Reserved deposit must reject another mine"
    placement = surface.get_placement_for_world_location(mine.get_actor_location(), mine.get_grid_footprint())
    assert not surface.can_occupy_cells(placement.origin_cell, mine.get_grid_footprint())
    unreal.log(f"STP_MINING_REGRESSION PASSED source={resource_name} attached=True otherBuildingBlocked=True duplicateBlocked=True")
    # Editor actors do not run EndPlay: explicitly release this fixture's grid cells.
    surface.release_cells(mine)
    source.release_mining_machine(mine)
    actors.destroy_actor(mine)
    actors.destroy_actor(preview)
    actors.destroy_actor(source)
unreal.log("STP_MINING_REGRESSION ALL PASSED")
