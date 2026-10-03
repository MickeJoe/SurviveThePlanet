"""Verify the actual 37-sector PIE world's contours block building placement."""
import json
import time
from pathlib import Path
import unreal

project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert 'L_PlanetClusters' in editor.get_editor_world().get_path_name()
started=time.monotonic()

def check(delta):
    if time.monotonic()-started<10: return
    try:
        world=editor.get_game_world()
        assert world,'PIE did not start'
        surface=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetSurfaceManager)[0]
        population=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)[0]
        assert not population.get_editor_property('diagnostics')
        sectors=population.get_editor_property('generated_clusters')
        assert len(sectors)==37
        checked=0
        sample=None
        for sector in sectors.values():
            template=sector.get_editor_property('sector_template')
            for slot in template.get_editor_property('cluster_slots'):
                point=slot.get_editor_property('shape').get_editor_property('points')[0]
                p=unreal.MathLibrary.transform_location(slot.get_editor_property('transform'),unreal.Vector(point.x,point.y,0))
                p=unreal.MathLibrary.transform_location(sector.get_actor_transform(),p)
                placement=surface.get_building_placement_for_world_location(p,unreal.IntPoint(1,1))
                assert not placement.valid,'Cluster boundary allowed building preview'
                assert not surface.has_building_clearance(placement.origin_cell,unreal.IntPoint(1,1),None)
                sector.clear_generated()
                assert not surface.has_building_clearance(placement.origin_cell,unreal.IntPoint(1,1),None),'Unloading removed blocker'
                sample=placement.origin_cell
                checked+=1
        # Open ground must not be blocked by a whole-sector bounding box.
        open_found=False
        for x in range(-2500,2501,500):
            for y in range(-2500,2501,500):
                if surface.get_building_placement_for_world_location(unreal.Vector(x,y,0),unreal.IntPoint(1,1)).valid:
                    open_found=True
                    break
            if open_found: break
        assert open_found,'No buildable open ground found'
        before=time.perf_counter()
        for i in range(1000): surface.has_building_clearance(sample,unreal.IntPoint(4,4),None)
        query_ms=(time.perf_counter()-before)
        report={'result':'passed','sectors':37,'slots_checked':checked,'unloaded_blockers':True,
                'open_ground_valid':True,'mean_query_ms_including_python':query_ms}
        (project/'Saved'/'ClusterBuildingPlacement.json').write_text(json.dumps(report,indent=2))
        unreal.log('CLUSTER_BUILDING_PLACEMENT_PASSED '+json.dumps(report))
    except Exception as error:
        (project/'Saved'/'ClusterBuildingPlacement.json').write_text(json.dumps({'error':str(error)}))
        unreal.log_error('CLUSTER_BUILDING_PLACEMENT_FAILED '+str(error))
    finally:
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()

handle=unreal.register_slate_post_tick_callback(check)
levels.editor_request_begin_play()
