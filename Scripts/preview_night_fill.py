import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
unreal.get_default_object(unreal.load_class(None,'/Script/PythonScriptPlugin.EditorPythonScriptingLibrary')).call_method('SetKeepPythonScriptAlive',(True,))
levels.editor_request_begin_play()
phase=0;start=time.monotonic()
def finish(error=None):
 unreal.unregister_slate_post_tick_callback(handle)
 (root/'Saved/NightFillPreview.json').write_text(json.dumps({'passed':error is None,'error':error}))
 levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(delta):
 global phase,start
 if time.monotonic()-start<4:return
 try:
  world=editor.get_game_world()
  if not world:return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  weather=unreal.GameplayStatics.get_actor_of_class(world,unreal.PlanetWeatherManager)
  if not pc or not weather:return
  if phase==0:
   state=unreal.PlanetWeatherState();state.precipitation_percent=0;state.sun_percent=0;state.wind_percent=3.8
   weather.set_weather_immediately(state);weather.update_presentation(1494)
   pawn=unreal.GameplayStatics.get_player_pawn(world,0);p=pawn.get_actor_location();pawn.set_actor_location(unreal.Vector(p.x+1500,p.y+700,p.z),False,False)
   boom=pawn.get_component_by_class(unreal.SpringArmComponent)
   if boom:boom.set_editor_property('target_arm_length',6000.0)
   phase=1;start=time.monotonic()
  elif phase==1:
   weather.update_presentation(1494)
   unreal.GameplayStatics.set_game_paused(world,True)
   unreal.SystemLibrary.execute_console_command(world,'shot showui',pc)
   phase=2;start=time.monotonic()
  else:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
