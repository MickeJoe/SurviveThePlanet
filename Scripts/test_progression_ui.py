"""Verify reward labels and the real delivery button inside gameplay PIE."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not editor.get_game_world()
subsystems=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
widgetlib=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
backups={p:p.read_bytes() for p in (root/'Saved/SaveGames').glob('*.sav')}
state={'stage':'start','started':time.monotonic(),'since':time.monotonic(),'checks':[]}
def transition(stage):state.update(stage=stage,since=time.monotonic())
def get_widgets(world,cls):return widgetlib.call_method('GetAllWidgetsOfClass',(world,cls,True))
def texts(widget):return [str(o.get_text()) for o in unreal.ObjectIterator(unreal.TextBlock) if o.get_path_name().startswith(widget.get_path_name())]
def capture(world,name):
    state['capture']=name
    unreal.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
def preserve():
    if not state.get('capture'):return
    files=list((root/'Saved/Screenshots').rglob('*.png'))
    assert files,'No screenshot saved'
    shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/('Progression_'+state.pop('capture')+'.png'))
def report(passed,error=None):
    for p,data in backups.items():p.write_bytes(data)
    value={'passed':passed,'checks':state['checks'],'error':error}
    (root/'Saved/ProgressionUIRuntime.json').write_text(json.dumps(value,indent=2))
    unreal.log('PROGRESSION_UI_RUNTIME '+json.dumps(value))
    unreal.unregister_slate_post_tick_callback(handle)
def tick(delta):
    if time.monotonic()-state['started']>180:report(False,'Timed out at '+state['stage']);return
    if time.monotonic()-state['since']<3:return
    try:
        stage=state['stage'];world=editor.get_game_world()
        if stage=='start':
            levels.editor_request_begin_play();transition('objective');return
        if not world:return
        mission=subsystems.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
        objectives=subsystems.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
        contracts=subsystems.call_method('GetWorldSubsystem',(world,unreal.ContractOfferSubsystem))
        if stage=='objective':
            mission.set_decay_paused(True)
            tracker=get_widgets(world,unreal.ObjectiveTrackerWidget)[0]
            assert '+1 MISSION PROGRESSION' in texts(tracker),texts(tracker)
            state['checks'].append('Active objective HUD displays its configured progression reward')
            capture(world,'Objectives');transition('choices');return
        if stage=='choices':
            preserve()
            objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
            assert mission.get_mission_progress()==1
            chooser=get_widgets(world,unreal.MissionChoiceWidget)[0]
            chooser.call_method('OpenPopup')
            buttons=[o for o in unreal.ObjectIterator(unreal.MissionChoiceButton) if o.get_path_name().startswith(chooser.get_path_name())]
            assert buttons
            buttons[0].call_method('HandleClicked')
            assert any('MISSION PROGRESSION' in s for s in texts(chooser)),texts(chooser)
            state['checks'].append('Objective picker previews per-objective progression rewards')
            state['chooser']=chooser;capture(world,'Choices');transition('offers');return
        if stage=='offers':
            preserve();state['chooser'].call_method('ClosePopup')
            objectives.confirm_mission_choices('mother_base_directives',['mission_deliver_copper','mission_build_storage','mission_expand_mining'])
            panel=get_widgets(world,unreal.TraderPanelWidget)[0]
            panel.call_method('HandleViewOffersRequested',('helix_industrial',))
            popup=get_widgets(world,unreal.ContractOfferWidget)[0]
            labels=texts(popup)
            for n in [1,2,3]:assert f'+{n} MISSION PROGRESSION' in labels,labels
            assert str(popup.get_class().get_path_name()).startswith('/Game/UI/Contracts/WBP_ContractOffer')
            state['checks'].append('WBP agreement offer displays +1/+2/+3 progression under completion rewards')
            state['popup']=popup;state['panel']=panel
            capture(world,'AgreementRewards');transition('active');return
        if stage=='active':
            preserve();state['popup'].call_method('HandleCloseClicked')
            assert contracts.toggle_contract_selection('helix_advanced_components','standard')
            assert contracts.confirm_selected_contract()
            manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
            manager.set_resource_amount(unreal.ResourceType.CONTROL_CHIP,32)
            state['manager']=manager
            transition('deliver');return
        if stage=='deliver':
            panel=state['panel']
            assert any('MISSION PROGRESSION ON COMPLETION' in s for s in texts(panel)),texts(panel)
            assert 'DELIVER SHIPMENT' in texts(panel),texts(panel)
            for index in range(4):
                panel.call_method('HandleDeliverContractClicked')
                assert mission.get_mission_progress()==(2 if index==3 else 1)
            assert contracts.get_selected_contract_state().state==unreal.STPContractState.COMPLETED
            assert state['manager'].get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==0
            state['checks'].append('Real UMG delivery handler consumes 4 shipments and grants completion progression only on the last one')
            transition('finished');return
        if stage=='finished':
            panel=state['panel'];assert any('4 / 4 DELIVERIES' in s for s in texts(panel)),texts(panel)
            capture(world,'CompletedAgreement');transition('stop');return
        if stage=='stop':
            preserve();unreal.EditorLevelLibrary.editor_end_play();report(True)
    except Exception:
        if editor.get_game_world():unreal.EditorLevelLibrary.editor_end_play()
        report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
print('PROGRESSION_UI_REVIEW_STARTED')
