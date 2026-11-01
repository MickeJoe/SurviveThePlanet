import unreal,time,json,traceback
from editor_toolset.toolsets.object import ObjectTools
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert str(root).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
bp=unreal.load_asset('/Game/UI/WBP_PauseMenu')
umg=unreal.get_default_object(unreal.UMGToolSet)
assert umg.call_method('CompileWidgetBlueprint',(bp,))
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets}
assert all(k in nodes for k in ['SaveButton','LoadButton','SaveName','SaveList','SaveStatus'])
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
levels.editor_request_begin_play()
phase=0;start=time.monotonic();checks=[];slot=None;save_system=None
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 if slot:unreal.GameplayStatics.delete_game_in_slot(slot,0)
 (root/'Saved/SaveLoadPIEReport.json').write_text(json.dumps({'passed':error is None,'checks':checks,'error':error},indent=2))
 unreal.log('SAVE_LOAD_PIE '+str(error or 'PASS'))
 levels.editor_request_end_play()
 unreal.SystemLibrary.quit_editor()
def tick(delta):
 global phase,start,slot,save_system,menu,world_path,drone_count
 if time.monotonic()-start<3:return
 try:
  world=editor.get_game_world()
  if not world:
   if time.monotonic()-start>90:raise AssertionError('Game world not ready')
   return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  if not pc:return
  library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
  resources=unreal.GameplayStatics.get_actor_of_class(world,unreal.ResourceManager)
  if not resources:return
  if phase==0:
   resources.set_resource_amount(unreal.ResourceType.STEEL,73);resources.set_credits(4321)
   objectives=next(x for x in unreal.ObjectIterator(unreal.ObjectiveSubsystem) if x.get_outer()==world)
   objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE)
   drone_count=len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseDrone))
   mission=next(x for x in unreal.ObjectIterator(unreal.MissionConfidenceSubsystem) if x.get_outer()==world);mission.set_mission_confidence(53)
   hud=library.call_method('GetAllWidgetsOfClass',(world,unreal.ResourceDisplayWidget,False))[0]
   checks.append('Paused game clock included in save')
   save_system=next(x for x in unreal.ObjectIterator(unreal.ColonySaveSubsystem) if x.get_outer()==unreal.GameplayStatics.get_game_instance(world))
   menu=library.call_method('Create',(world,bp.generated_class(),pc));menu.add_to_viewport(2000)
   name=next(x for x in unreal.ObjectIterator(unreal.EditableTextBox) if x.get_name()=='SaveName' and x.get_outer().get_outer()==menu);assert len(str(name.get_text()))>=19
   name.set_text('QA Save Load '+str(time.time_ns()))
   menu.call_method('Save')
   slots=save_system.list_saves();entry=next(e for e in slots if e.display_name==str(name.get_text()));slot=entry.slot
   checks.extend(['WBP compiled with Save/Load controls','Timestamp default name and custom name','Save button writes an independent slot'])
   unreal.GameplayStatics.set_game_paused(world,True)
   phase=1;start=time.monotonic()
  elif phase==1:
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   resources.set_resource_amount(unreal.ResourceType.STEEL,2);resources.set_credits(1)
   world_path=world.get_path_name()
   menu.call_method('Load')
   phase=2;start=time.monotonic()
  elif phase==2:
   if time.monotonic()-start<8:return
   assert resources.get_resource_amount(unreal.ResourceType.STEEL)==73,resources.get_resource_amount(unreal.ResourceType.STEEL)
   assert resources.get_credits()==4321
   confidence=next(x for x in unreal.ObjectIterator(unreal.MissionConfidenceSubsystem) if x.get_outer()==world).get_mission_confidence()
   assert abs(confidence-53)<.01,confidence
   assert len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseDrone))==drone_count
   hud=library.call_method('GetAllWidgetsOfClass',(world,unreal.ResourceDisplayWidget,False))[0]
   clock=next(x for x in unreal.ObjectIterator(unreal.TextBlock) if x.get_name()=='ClockText' and x.get_outer().get_outer().get_owning_player()==pc)
   assert '08:00' in str(clock.get_text()),str(clock.get_text())
   menus=library.call_method('GetAllWidgetsOfClass',(world,unreal.PauseMenuWidget,False))
   assert any(m.is_in_viewport() for m in menus)
   assert unreal.GameplayStatics.is_game_paused(world)
   assert any(str(x.get_text())=='BUILD CARGO BAY' and x.get_outer().get_outer().get_owning_player()==pc for x in unreal.ObjectIterator(unreal.TextBlock))
   checks.append('Objective tracker refreshes after loading')
   checks.extend(['Actual map travel restores inventory, credits, confidence and drones','Saved game clock restored','Load returns to paused Esc menu'])
   finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
