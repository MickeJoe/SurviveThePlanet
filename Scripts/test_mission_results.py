"""Exercise real PIE outcomes, rewards, HUD binding, retry and planet selection."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not editor.get_game_world()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
backups={p:p.read_bytes() for p in (root/'Saved/SaveGames').glob('*.sav')}
widgets=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
subsystems=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
state={'stage':'start','since':time.monotonic(),'started':time.monotonic(),'checks':[]}
def all_widgets(world,cls):return widgets.call_method('GetAllWidgetsOfClass',(world,cls,True))
def child(widget,name):
    return next(o for o in unreal.ObjectIterator(unreal.Widget) if o.get_name()==name and o.get_path_name().startswith(widget.get_path_name()))
def transition(stage):state.update(stage=stage,since=time.monotonic())
def report(passed,error=None):
    for p,data in backups.items():p.write_bytes(data)
    value={'passed':passed,'checks':state['checks'],'error':error}
    (root/'Saved/MissionResultsRuntime.json').write_text(json.dumps(value,indent=2))
    unreal.log('MISSION_RESULTS_RUNTIME '+json.dumps(value))
    unreal.unregister_slate_post_tick_callback(handle)
def capture(world,label):
    state['capture']=label
    unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
def preserve():
    files=list((root/'Saved/Screenshots').rglob('*.png'))
    if files and state.get('capture'):
        shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/('MissionResult_'+state.pop('capture')+'.png'))
def tick(delta):
    if time.monotonic()-state['started']>240:report(False,'Timed out at '+state['stage']);return
    if time.monotonic()-state['since']<3:return
    world=editor.get_game_world()
    try:
        stage=state['stage']
        if stage=='start':
            levels.load_level('/Game/Planet/Maps/L_Nexaris')
            levels.editor_request_begin_play();transition('win');return
        if not world:return
        if stage in ('win','fail'):
            mission=subsystems.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
            manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
            mission.set_decay_paused(True)
            mission.set_mission_progress(9)
            assert mission.get_mission_outcome()==unreal.MissionOutcome.IN_PROGRESS
            hud=all_widgets(world,unreal.MissionConfidenceWidget)[0]
            assert abs(child(hud,'MissionProgressBar').percent-.9)<.001
            state['checks'].append('HUD reads authoritative progress; 9/10 does not end mission ('+stage+')')
            balance=manager.get_credits();steel=manager.get_resource_amount(unreal.ResourceType.STEEL)
            if stage=='win':mission.add_mission_progress(1)
            else:mission.set_mission_confidence(0)
            expected=unreal.MissionOutcome.COMPLETE if stage=='win' else unreal.MissionOutcome.FAILED
            assert mission.get_mission_outcome()==expected
            results=all_widgets(world,unreal.MissionResultWidget)
            assert len(results)==1 and results[0].is_in_viewport()
            result=results[0]
            assert str(child(result,'OutcomeText').get_text())==('MISSION COMPLETE' if stage=='win' else 'MISSION FAILED')
            assert unreal.GameplayStatics.is_game_paused(world)
            assert manager.get_credits()==balance+(2500 if stage=='win' else 450)
            assert manager.get_resource_amount(unreal.ResourceType.STEEL)==steel+(150 if stage=='win' else 30)
            balance=manager.get_credits()
            mission.set_mission_progress(10);mission.set_mission_confidence(0)
            assert mission.get_mission_outcome()==expected and manager.get_credits()==balance
            state['checks'].append(stage+': correct result WBP, paused gameplay, exact credit/resource rewards once, immutable outcome')
            state['result']=result
            capture(world,stage)
            transition('retry' if stage=='win' else 'select');return
        if stage=='retry':
            preserve();state['result'].call_method('Retry');transition('retry_check');return
        if stage=='retry_check':
            mission=subsystems.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
            assert mission.get_mission_outcome()==unreal.MissionOutcome.IN_PROGRESS
            assert mission.get_mission_progress()==0
            assert not unreal.GameplayStatics.is_game_paused(world)
            state['checks'].append('Play Again reloads current planet and resets outcome/progress')
            transition('fail');return
        if stage=='select':
            preserve();state['result'].call_method('SelectPlanet');transition('selector_check');return
        if stage=='selector_check':
            assert 'L_PlanetSelection' in world.get_path_name()
            assert not unreal.GameplayStatics.is_game_paused(world)
            state['checks'].append('Planet Selector returns to the selection map and unpauses')
            unreal.EditorLevelLibrary.editor_end_play();report(True);return
    except Exception:
        unreal.EditorLevelLibrary.editor_end_play();report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
print('MISSION_RESULT_RUNTIME_STARTED')
