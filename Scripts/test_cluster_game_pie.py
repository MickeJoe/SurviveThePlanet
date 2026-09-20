"""Begin actual PIE and validate the generated runtime world asynchronously."""
import hashlib
import json
import time
from pathlib import Path
import unreal

project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assert 'L_PlanetClusters' in editor.get_editor_world().get_path_name()
# This test checks every composition, independently of camera residency.
source_population=unreal.GameplayStatics.get_all_actors_of_class(editor.get_editor_world(),unreal.SectorPopulation)[0]
previous_residency=source_population.get_editor_property('manage_cluster_residency')
source_population.set_editor_property('manage_cluster_residency',False)
state={'started':time.monotonic()}

def check(delta):
    if time.monotonic()-state['started']>120:
        source_population.set_editor_property('manage_cluster_residency',previous_residency)
        unreal.unregister_slate_post_tick_callback(handle)
        (project/'Saved'/'ClusterGamePIE.json').write_text(json.dumps({'error':'PIE startup timed out'}))
        return
    world=editor.get_game_world()
    if not world or time.monotonic()-state['started']<8: return
    try:
        grids=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HexSectorGrid)
        populations=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)
        assert len(grids)==len(populations)==1
        grid,population=grids[0],populations[0]
        generated=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetGeneratedSector)
        diagnostics=[str(v) for v in population.get_editor_property('diagnostics')]
        assert not diagnostics,diagnostics
        assert len(generated)==len(grid.get_editor_property('sectors'))==37,len(generated)
        assert len(population.get_editor_property('decorations'))==0,'Legacy scatter also active'
        rows=[]
        for sector_id,actor in population.get_editor_property('generated_clusters').items():
            selected=[str(v) for v in actor.get_editor_property('selected_variant_ids')]
            assert len(selected)==6 and 'None' not in selected
            assert not actor.get_editor_property('diagnostics')
            values=[]
            for c in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
                for i in range(c.get_instance_count()):
                    t=c.get_instance_transform(i,True)
                    p,q,s=t.translation,t.rotation,t.scale3d
                    values.append((c.static_mesh.get_path_name(),p.x,p.y,p.z,q.x,q.y,q.z,q.w,s.x,s.y,s.z))
            assert len(values)>700
            rows.append({'sector_id':sector_id,'seed':actor.get_editor_property('seed'),'selected':selected,'instances':len(values),'visible':not actor.get_editor_property('hidden'),'transform_hash':hashlib.sha256(json.dumps(sorted(values)).encode()).hexdigest()})
        assert len({r['seed'] for r in rows})==37
        assert len({tuple(r['selected']) for r in rows})>1
        start=grid.get_editor_property('starting_sector_id')
        assert next(r for r in rows if r['sector_id']==start)['visible']
        hidden=next(r for r in rows if r['sector_id']!=start)
        assert not hidden['visible']
        grid.set_sector_state(hidden['sector_id'],unreal.SectorState.DISCOVERED)
        assert not population.get_editor_property('generated_clusters')[hidden['sector_id']].get_editor_property('hidden')
        controller=unreal.GameplayStatics.get_player_controller(world,0)
        pawn=unreal.GameplayStatics.get_player_pawn(world,0)
        assert controller and pawn,'No player pawn in PIE'
        report={'result':'passed','sector_count':37,'instances':sum(r['instances'] for r in rows),'distinct_layouts':len({tuple(r['selected']) for r in rows}),'no_legacy_scatter':True,'discovery_reveals_clusters':True,'pawn':pawn.get_class().get_name(),'sectors':sorted(rows,key=lambda r:r['sector_id'])}
        previous=project/'Saved'/'ClusterGamePIE.json'
        if previous.exists():
            old=json.loads(previous.read_text())
            if old.get('result')=='passed':
                assert old['sectors']==report['sectors'],'Second PIE run was not deterministic'
                report['repeat_pie_determinism']='passed'
        previous.write_text(json.dumps(report,indent=2))
        unreal.log('CLUSTER_GAME_PIE_PASSED '+str(report['instances']))
    except Exception as error:
        (project/'Saved'/'ClusterGamePIE.json').write_text(json.dumps({'error':str(error)}))
        unreal.log_error('CLUSTER_GAME_PIE_FAILED '+str(error))
    finally:
        source_population.set_editor_property('manage_cluster_residency',previous_residency)
        unreal.unregister_slate_post_tick_callback(handle)

handle=unreal.register_slate_post_tick_callback(check)
levels.editor_request_begin_play()
