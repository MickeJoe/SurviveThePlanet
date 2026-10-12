import unreal, json, time, traceback
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world
weather=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetWeatherManager)[0]
plant=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.GeothermalPlant)[0]
pawn=unreal.GameplayStatics.get_player_pawn(world,0)
# Move the existing gameplay camera so the compact plant sits in the open center.
pawn.set_actor_location(plant.get_actor_location()+unreal.Vector(-650,300,0),False,False)
position=plant.get_actor_location()+unreal.Vector(0,430,0)
position.z=state['site'].get_actor_location().z
if 'nearby' in globals() and unreal.is_valid(nearby):nearby.destroy_actor()
nearby=spawn(world,unreal.GeothermalSite,position)
for emitter in plant.get_components_by_class(unreal.NiagaraComponent):emitter.reinitialize_system()
unreal.GameplayStatics.set_game_paused(world,False)
unreal.GameplayStatics.set_global_time_dilation(world,1)
weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=12,sun_percent=85))
day_minutes=weather.get_planet_definition().get_day_length_minutes()
midday=max(range(int(day_minutes)),key=lambda t:weather.get_planet_definition().get_solar_elevation(t))
night=min(range(int(day_minutes)),key=lambda t:weather.get_planet_definition().get_solar_elevation(t))
weather.update_presentation(midday)
check={'start':time.monotonic(),'stage':0,'midday':midday,'night':night}
def capture_tick(dt):
 try:
  if time.monotonic()-check['start']<6:return
  if check['stage']==0:
   steam=plant.get_components_by_class(unreal.NiagaraComponent)
   assert all(c.is_active() for c in steam),'Operational plant must emit steam'
   winds=[c.get_variable_vec3('User.Wind') for c in steam]
   assert all(v[1] and v[0].x>0 for v in winds),'Planet wind must reach Niagara'
   check['day_sun']=weather.get_current_weather().sun_percent
   check['day_lamps']=[c.get_editor_property('intensity') for c in plant.get_components_by_class(unreal.PointLightComponent) if c.get_editor_property('attenuation_radius')==135]
   unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
   check['stage']=1
  elif check['stage']==1:
   files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'))
   import shutil
   shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved/GeothermalDay.png')
   weather.update_presentation(night)
   check['stage']=2
  elif check['stage']==2:
   check['night_sun']=weather.get_current_weather().sun_percent
   check['night_lamps']=[c.get_editor_property('intensity') for c in plant.get_components_by_class(unreal.PointLightComponent) if c.get_editor_property('attenuation_radius')==135]
   assert check['night_sun']==0
   assert len(check['night_lamps'])==4 and all(i>0 for i in check['night_lamps'])
   assert all(i==0 for i in check['day_lamps'])
   unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
   check['stage']=3
  else:
   files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'))
   import shutil
   shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved/GeothermalNight.png')
   check['passed']=True
   (root/'Saved/GeothermalVisualChecks.json').write_text(json.dumps(check,indent=2))
   unreal.unregister_slate_post_tick_callback(visual_handle)
  check['start']=time.monotonic()
 except Exception:
  check['passed']=False;check['error']=traceback.format_exc()
  (root/'Saved/GeothermalVisualChecks.json').write_text(json.dumps(check,indent=2))
  unreal.unregister_slate_post_tick_callback(visual_handle)
visual_handle=unreal.register_slate_post_tick_callback(capture_tick)
