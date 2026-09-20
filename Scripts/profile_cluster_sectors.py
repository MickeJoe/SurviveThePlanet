"""Run in the editor Python console. Compare identical 1/7/37-sector camera paths.

Writes frame-time samples, mesh statistics and CSV GPU/Game/Render timings to Saved.
No level/asset changes are saved. Run once before and once after optimization.
"""
import json
import sys
import time
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower() == r'C:\UE5\SurviveThePlanet 5.8'.lower()
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
source_population = unreal.GameplayStatics.get_all_actors_of_class(editor.get_editor_world(), unreal.SectorPopulation)[0]
previous_policy = (source_population.get_editor_property('manage_cluster_residency'),
                   source_population.get_editor_property('optimize_cluster_rendering'))
baseline = '--baseline' in sys.argv
source_population.set_editor_property('manage_cluster_residency', not baseline)
source_population.set_editor_property('optimize_cluster_rendering', not baseline)
settings = unreal.get_default_object(unreal.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
previous_throttle = settings.get_editor_property('bThrottleCPUWhenNotForeground')
settings.set_editor_property('bThrottleCPUWhenNotForeground', False)
state = {'phase': -1, 'since': time.monotonic(), 'samples': [], 'rows': [], 'revealed': set()}
previous_max_fps = unreal.SystemLibrary.get_console_variable_float_value('t.MaxFPS')
previous_vsync = unreal.SystemLibrary.get_console_variable_int_value('r.VSync')
output = project / 'Saved' / 'ClusterPerformance.json'
output.write_text(json.dumps({'result': 'running'}))


def command(world, text):
    unreal.SystemLibrary.execute_console_command(world, text)


def restore_policy():
    source_population.set_editor_property('manage_cluster_residency', previous_policy[0])
    source_population.set_editor_property('optimize_cluster_rendering', previous_policy[1])


def tick(delta):
    try:
        now = time.monotonic()
        frame_ms = (now - state.get('last_tick', now)) * 1000
        state['last_tick'] = now
        world = editor.get_game_world()
        if not world:
            if time.monotonic() - state['since'] > 120:
                raise RuntimeError('No PIE world after 120 seconds')
            return
        if state['phase'] == -1:
            if time.monotonic() - state['since'] < 15:
                return
            state['population'] = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SectorPopulation)[0]
            state['grid'] = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HexSectorGrid)[0]
            state['pawn'] = unreal.GameplayStatics.get_player_pawn(world, 0)
            state['sectors'] = sorted(state['grid'].get_editor_property('sectors'),
                                      key=lambda s: s.world_center.length_squared())
            state['origin'] = state['pawn'].get_actor_location()
            state['phase'] = 0
            state['since'] = time.monotonic()
            command(world, 't.MaxFPS 0')
            command(world, 'r.VSync 0')
            command(world, 'csvprofile start')
            return
        elapsed = time.monotonic() - state['since']
        counts = [1, 7, 37]
        count = counts[state['phase']]
        for sector in state['sectors'][:count]:
            if sector.id not in state['revealed']:
                state['grid'].set_sector_state(sector.id, unreal.SectorState.DISCOVERED)
                state['revealed'].add(sector.id)
        # Same local path for all discovery counts; do not compare different views.
        import math
        angle = max(0, elapsed - 8) * 0.4
        origin = state['origin']
        state['pawn'].set_actor_location(unreal.Vector(origin.x + 1800 * math.sin(angle),
                                                       origin.y + 1800 * (1 - math.cos(angle)), origin.z), False, False)
        if elapsed > 8:
            state['samples'].append(frame_ms)
        if elapsed < 24:
            return
        actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlanetGeneratedSector)
        components = [c for a in actors for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent)]
        samples = sorted(state['samples'])
        row = {'discovered': count, 'frames': len(samples), 'median_ms': samples[len(samples)//2],
               'p95_ms': samples[int(len(samples)*0.95)], 'mean_ms': sum(samples)/len(samples),
               'instances': sum(c.get_instance_count() for c in components), 'components': len(components)}
        state['rows'].append(row)
        unreal.log('CLUSTER_PERFORMANCE ' + json.dumps(row))
        state['phase'] += 1
        state['samples'] = []
        state['since'] = time.monotonic()
        if state['phase'] == len(counts):
            command(world, 'csvprofile stop')
            command(world, 't.MaxFPS ' + str(previous_max_fps))
            command(world, 'r.VSync ' + str(previous_vsync))
            output.write_text(json.dumps({'result': 'complete', 'baseline_policy': baseline, 'rows': state['rows'],
                'note': 'PIE wall-clock frame deltas; use accompanying CSV for GPU/Game/Render attribution'}, indent=2))
            unreal.unregister_slate_post_tick_callback(handle)
            settings.set_editor_property('bThrottleCPUWhenNotForeground', previous_throttle)
            restore_policy()
            levels.editor_request_end_play()
    except Exception as error:
        output.write_text(json.dumps({'error': str(error), 'rows': state['rows']}, indent=2))
        unreal.log_error(str(error))
        unreal.unregister_slate_post_tick_callback(handle)
        settings.set_editor_property('bThrottleCPUWhenNotForeground', previous_throttle)
        restore_policy()
        if editor.get_game_world():
            command(editor.get_game_world(), 'csvprofile stop')
            command(editor.get_game_world(), 't.MaxFPS ' + str(previous_max_fps))
            command(editor.get_game_world(), 'r.VSync ' + str(previous_vsync))
            levels.editor_request_end_play()


handle = unreal.register_slate_post_tick_callback(tick)
levels.editor_request_begin_play()
