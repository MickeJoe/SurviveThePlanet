"""PIE regression: discovery, eviction/reload, exact seeds/transforms, resources."""
import hashlib
import json
import sys
import time
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower() == r'C:\UE5\SurviveThePlanet 5.8'.lower()
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
settings = unreal.get_default_object(unreal.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
previous_throttle = settings.get_editor_property('bThrottleCPUWhenNotForeground')
settings.set_editor_property('bThrottleCPUWhenNotForeground', False)
state = {'stage': 0, 'since': time.monotonic()}
output = project / 'Saved' / 'ClusterResidencyTest.json'
profile_only = '--profile' in sys.argv
if profile_only:
    output = project / 'Saved' / 'ClusterTraversalProfile.json'
output.write_text(json.dumps({'result': 'running'}))


def signature(actor):
    values = []
    for component in actor.get_components_by_class(unreal.InstancedStaticMeshComponent):
        for index in range(component.get_instance_count()):
            transform = component.get_instance_transform(index, True)
            p, q, s = transform.translation, transform.rotation, transform.scale3d
            values.append((component.static_mesh.get_path_name(), p.x, p.y, p.z,
                           q.x, q.y, q.z, q.w, s.x, s.y, s.z))
    return hashlib.sha256(json.dumps(sorted(values)).encode()).hexdigest(), len(values)


def tick(delta):
    try:
        if time.monotonic() - state['since'] < (2 if state['stage'] >= 3 else 10):
            return
        world = editor.get_game_world()
        assert world, 'PIE world missing'
        if state['stage'] == 0:
            population = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SectorPopulation)[0]
            grid = population.get_editor_property('grid')
            actors = population.get_editor_property('generated_clusters')
            assert len(actors) == len(grid.get_editor_property('sectors')) == 37
            assert not population.get_editor_property('diagnostics')
            resident = [a for a in actors.values() if a.get_editor_property('resident')]
            assert 0 < len(resident) < 37, len(resident)
            start = grid.get_editor_property('starting_sector_id')
            target = actors[start]
            initial = signature(target)
            assert initial[1] > 700
            for sector in grid.get_editor_property('sectors'):
                grid.set_sector_state(sector.id, unreal.SectorState.DISCOVERED)
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            origin = pawn.get_actor_location()
            state.update(population=population, grid=grid, actors=actors, pawn=pawn,
                         origin=origin, target=target, initial=initial,
                         deposits=len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.BaseResourceSource)))
            pawn.set_actor_location(unreal.Vector(origin.x + 100000, origin.y + 100000, origin.z), False, False)
        elif state['stage'] == 1:
            assert not state['target'].get_editor_property('resident'), 'Offscreen cluster was not evicted'
            assert signature(state['target'])[1] == 0
            state['pawn'].set_actor_location(state['origin'], False, False)
        elif state['stage'] == 2:
            assert state['target'].get_editor_property('resident'), 'Returning camera did not reload cluster'
            assert signature(state['target']) == state['initial'], 'Regeneration changed exact composition'
            assert all(s.state != unreal.SectorState.UNDISCOVERED for s in state['grid'].get_editor_property('sectors'))
            assert len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.BaseResourceSource)) == state['deposits']
            resident = [a for a in state['actors'].values() if a.get_editor_property('resident')]
            assert len(resident) < 37
            report = {'result': 'passed', 'all_sectors': 37, 'resident_after_return': len(resident),
                      'resident_instances': sum(signature(a)[1] for a in resident),
                      'exact_reload_hash': state['initial'][0], 'discovery_preserved': True,
                      'resource_actors_preserved': state['deposits']}
            state['report'] = report
            state['visit_ids'] = sorted(state['actors'])
            state['visited'] = 0
            state['expected'] = {r['sector_id']: r['transform_hash'] for r in
                                 json.loads((project/'Saved'/'SectorCatalogPIE.json').read_text())['sectors']}
            unreal.SystemLibrary.execute_console_command(world, 'csvprofile start')
            state['pawn'].get_component_by_class(unreal.SpringArmComponent).set_editor_property('target_arm_length', 6000)
            target_location = state['actors'][state['visit_ids'][0]].get_actor_location()
            state['pawn'].set_actor_location(target_location, False, False)
        else:
            sector_id = state['visit_ids'][state['visited']]
            target = state['actors'][sector_id]
            assert target.get_editor_property('resident'), ('Visible sector missing', sector_id)
            if not profile_only:
                assert signature(target)[0] == state['expected'][sector_id], ('Composition changed', sector_id)
            state['visited'] += 1
            if state['visited'] == len(state['visit_ids']):
                state['report']['max_zoom_sectors_visited'] = state['visited']
                state['report']['hash_checks_during_capture'] = not profile_only
                output.write_text(json.dumps(state['report'], indent=2))
                unreal.log('CLUSTER_RESIDENCY_PASSED ' + json.dumps(state['report']))
                unreal.unregister_slate_post_tick_callback(handle)
                unreal.SystemLibrary.execute_console_command(world, 'csvprofile stop')
                settings.set_editor_property('bThrottleCPUWhenNotForeground', previous_throttle)
                levels.editor_request_end_play()
                return
            target_location = state['actors'][state['visit_ids'][state['visited']]].get_actor_location()
            state['pawn'].set_actor_location(target_location, False, False)
        state['stage'] += 1
        state['since'] = time.monotonic()
    except Exception as error:
        output.write_text(json.dumps({'error': str(error), 'stage': state['stage']}))
        unreal.log_error(str(error))
        unreal.unregister_slate_post_tick_callback(handle)
        if editor.get_game_world():
            unreal.SystemLibrary.execute_console_command(editor.get_game_world(), 'csvprofile stop')
        settings.set_editor_property('bThrottleCPUWhenNotForeground', previous_throttle)
        levels.editor_request_end_play()


handle = unreal.register_slate_post_tick_callback(tick)
levels.editor_request_begin_play()
