import unreal,time,json,traceback,hashlib
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
slot='Colony_EE5967244B508DB8DB0489A0449AD0C3'
path=root/'Saved/SaveGames'/f'{slot}.sav'
original_hash=hashlib.sha256(path.read_bytes()).hexdigest()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
levels.editor_request_begin_play()
phase=0;start=time.monotonic();checks=[];details={}
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 details['original_save_unchanged']=hashlib.sha256(path.read_bytes()).hexdigest()==original_hash
 report={'passed':error is None,'checks':checks,'details':details,'error':error}
 (root/'Saved/ExistingSaveSelectorReport.json').write_text(json.dumps(report,indent=2))
 unreal.log('EXISTING_SAVE_SELECTOR '+str(error or 'PASS'))
 levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(delta):
 global phase,start,menu,saved
 if time.monotonic()-start<3:return
 try:
  world=editor.get_game_world()
  if not world:
   if time.monotonic()-start>120:raise AssertionError('Game world not ready')
   return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  if not pc:return
  if phase==0:
   selectors=library.call_method('GetAllWidgetsOfClass',(world,unreal.PlanetSelectorWidget,False))
   assert selectors,'Planet selector not visible'
   selector=selectors[0];selector.open_pause_menu()
   menu=library.call_method('GetAllWidgetsOfClass',(world,unreal.PauseMenuWidget,False))[0]
   widgets=lambda cls,name:next(x for x in unreal.ObjectIterator(cls) if x.get_name()==name and x.get_outer().get_outer()==menu)
   assert not widgets(unreal.Button,'SaveButton').get_is_enabled()
   assert widgets(unreal.Button,'LoadButton').get_is_enabled()
   saved=unreal.GameplayStatics.load_game_from_slot(slot,0);assert saved
   system=next(x for x in unreal.ObjectIterator(unreal.ColonySaveSubsystem) if x.get_outer()==unreal.GameplayStatics.get_game_instance(world))
   entries=system.list_saves();index=next(i for i,e in enumerate(entries) if e.slot==slot)
   widgets(unreal.ComboBoxString,'SaveList').set_selected_index(index)
   checks.extend(['Planet selection Esc menu opens','Save disabled without active colony','Existing saves selectable in Load list'])
   phase=1;start=time.monotonic()
  elif phase==1:
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   menu.call_method('Load');phase=2;start=time.monotonic()
  elif phase==2:
   if time.monotonic()-start<12:return
   system=next(x for x in unreal.ObjectIterator(unreal.ColonySaveSubsystem) if x.get_outer()==unreal.GameplayStatics.get_game_instance(world))
   status=str(system.get_editor_property('LastLoadStatus'));assert status=='Game loaded.',status
   connections=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding)
   count=0
   for building in connections:
    for component in building.get_components_by_class(unreal.EnergyConnectionComponent):
     poles=[x for x in building.get_components_by_class(unreal.InstancedStaticMeshComponent) if x.get_name().startswith('InstancedStaticMeshComponent') and x.is_visible() and x.get_instance_count()>0]
     spans=[x for x in building.get_components_by_class(unreal.SplineMeshComponent) if x.is_visible()]
     details.setdefault('restored_connections',[]).append({'owner':building.get_name(),'component':component.get_name(),'visible_pole_groups':len(poles),'visible_spans':len(spans)})
     if spans:count+=1
   assert count>0,'Saved power connections have no visible spans'
   assert unreal.GameplayStatics.is_game_paused(world)
   checks.extend(['User existing save loads successfully from planet selector','Saved runtime power connections restored with visible cable spans','Loaded game returns paused'])
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   phase=3;start=time.monotonic()
  elif phase==3:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
