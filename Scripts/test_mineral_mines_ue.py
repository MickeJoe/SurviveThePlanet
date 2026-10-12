import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Nexaris')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]};types=[unreal.ResourceType.SILICA,unreal.ResourceType.RARE_MINERALS]
def actors(w,c):return unreal.GameplayStatics.get_all_actors_of_class(w,c)
def spawn(w,cls,t,preview=False):
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(w,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 if preview:a.set_placement_preview(True)
 return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def finish(passed,error=None):
 (root/'Saved/MineralMinesReview.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
def capture(name):
 files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def tick(dt):
 if state['stage'] and time.monotonic()-state['started']>180:finish(False,'Timeout');return
 if 'weather' in state:state['weather'].update_presentation(state['night'] if state['stage']>=3 else state['day'])
 if time.monotonic()-state['since']<(12 if state['stage'] in [1,3] else 4):return
 try:
  w=editor.get_game_world()
  if not w:return
  if state['stage']==0:
   state['started']=time.monotonic();pop=actors(w,unreal.SectorPopulation)[0];grid=actors(w,unreal.HexSectorGrid)[0]
   starts=[s for s in actors(w,unreal.BaseResourceSource) if s.get_resource_type() in types];assert len(starts)==2
   for d in pop.resources.deposits:
    if d.resource_type in types:pop.reveal_sector(d.sector_id)
   sources=[s for s in actors(w,unreal.BaseResourceSource) if s.get_resource_type() in types]
   for source in sources:assert unreal.get_default_object(source.get_mine_blueprint()).can_mine_resource_source(source),source.get_name()
   state['checks'].append('Both refined mines fit all generated silica/rare-mineral deposits')
   manager=actors(w,unreal.ResourceManager)[0];droneclass=unreal.load_asset('/Game/BluePrints/MiningDrone/BP_MiningDrone').generated_class();fixtures=[]
   for source in starts:
    defaults=unreal.get_default_object(source.get_mine_blueprint());assert all(not defaults.can_mine_resource_source(other) for other in starts if other!=source)
    t=defaults.get_placement_transform_for_source(source);preview=spawn(w,source.get_mine_blueprint(),t,True);assert not preview.get_components_by_class(unreal.NiagaraComponent);preview.destroy_actor()
    mine=spawn(w,source.get_mine_blueprint(),t);assert mine.attach_to_resource_source(source);mine.set_construction_progress(1)
    assert mine.get_max_drone_slots()==4 and mine.get_unlocked_drone_slots()==1;assert not mine.is_producing_resource()
    body=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh');assert 'Refined' in body.get_editor_property('static_mesh').get_name()
    drill=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name().startswith('MineDrill'));fan=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name().startswith('Motion'))
    drone=spawn(w,droneclass,unreal.Transform(location=mine.get_actor_location()+unreal.Vector(170,0,180)));assert mine.try_assign_drone(drone,0)
    fixtures.append({'mine':mine,'source':source,'drill':drill,'fan':fan,'drill_rotation':str(drill.get_editor_property('relative_rotation')),'drill_location':str(drill.get_editor_property('relative_location')),'fan_rotation':str(fan.get_editor_property('relative_rotation')),'amount':source.get_remaining_amount(),'stock':manager.get_resource_amount(source.get_resource_type()),'drones':[drone]})
   state.update(fixtures=fixtures,manager=manager,droneclass=droneclass)
   center=(fixtures[0]['mine'].get_actor_location()+fixtures[1]['mine'].get_actor_location())*.5;pawn=unreal.GameplayStatics.get_player_pawn(w,0);pawn.set_actor_location(center,False,False);pawn.set_actor_tick_enabled(False)
   weather=actors(w,unreal.PlanetWeatherManager)[0];planet=weather.get_planet_definition();minutes=int(planet.get_day_length_minutes());weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=85))
   state.update(weather=weather,day=max(range(minutes),key=planet.get_solar_elevation),night=min(range(minutes),key=planet.get_solar_elevation))
   unreal.GameplayStatics.set_game_paused(w,False);unreal.GameplayStatics.set_global_time_dilation(w,1);state['stage']=1
  elif state['stage']==1:
   for f in state['fixtures']:
    mine=f['mine'];source=f['source'];assert mine.is_producing_resource();assert source.get_remaining_amount()<f['amount'];assert f['amount']-source.get_remaining_amount()==state['manager'].get_resource_amount(source.get_resource_type())-f['stock']
    assert str(f['drill'].get_editor_property('relative_rotation'))!=f['drill_rotation'];assert str(f['drill'].get_editor_property('relative_location'))!=f['drill_location'];assert str(f['fan'].get_editor_property('relative_rotation'))!=f['fan_rotation']
    assert any(c.is_active() for c in mine.get_components_by_class(unreal.NiagaraComponent));f['one_drone_rate']=mine.get_current_output_per_minute()
    body=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh');pos=mine.get_actor_location();hit=unreal.SystemLibrary.line_trace_single(w,pos+unreal.Vector(0,0,1000),pos-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,actors(w,unreal.BaseBuilding)+actors(w,unreal.BaseResourceSource),unreal.DrawDebugTrace.NONE);assert hit
    ground=statics.call_method('BreakHitResult',(hit,))[5];bottom=body.get_world_location().z+body.get_editor_property('static_mesh').get_bounding_box().min.z*body.get_world_scale().z;assert abs(bottom-ground.z)<2,('Floating foundation',bottom-ground.z)
   state['checks'].append('One drone produces the correct resource; drill rotates/feeds, fan turns, dust runs, foundations sit on terrain')
   unreal.SystemLibrary.execute_console_command(w,'HighResShot 1920x1080');state['stage']=2
  elif state['stage']==2:
   capture('MineralMinesDay.png')
   for f in state['fixtures']:
    mine=f['mine'];mine.set_unlocked_drone_slots(4)
    for i in range(1,4):
     drone=spawn(w,state['droneclass'],unreal.Transform(location=mine.get_actor_location()+unreal.Vector(170,i*60,180)));assert mine.try_assign_drone(drone,i);f['drones'].append(drone)
    assert mine.get_assigned_drone_count()==4;assert abs(mine.get_current_output_per_minute()-4*f['one_drone_rate'])<.01;f['four_amount']=f['source'].get_remaining_amount()
   state['stage']=3
  elif state['stage']==3:
   for f in state['fixtures']:
    assert f['source'].get_remaining_amount()<f['four_amount'];assert any(c.get_editor_property('intensity')>0 for c in f['mine'].get_components_by_class(unreal.PointLightComponent))
   state['checks'].append('Four drones scale extraction correctly; night worklights illuminate both machines')
   unreal.SystemLibrary.execute_console_command(w,'HighResShot 1920x1080');state['stage']=4
  elif state['stage']==4:
   capture('MineralMinesNight.png')
   for f in state['fixtures']:
    for drone in f['drones']:assert f['mine'].unassign_drone(drone)
    assert not f['mine'].is_producing_resource()
   state['stage']=5
  elif state['stage']==5:
   for f in state['fixtures']:
    assert not any(c.is_active() for c in f['mine'].get_components_by_class(unreal.NiagaraComponent));f['idle_drill']=str(f['drill'].get_editor_property('relative_rotation'));f['idle_fan']=str(f['fan'].get_editor_property('relative_rotation'))
   state['stage']=6
  else:
   for f in state['fixtures']:
    assert str(f['drill'].get_editor_property('relative_rotation'))==f['idle_drill'];assert str(f['fan'].get_editor_property('relative_rotation'))==f['idle_fan']
    assert f['source'].extract_resource(f['source'].get_remaining_amount())>0;assert f['mine'].try_assign_drone(f['drones'][0],0);assert not f['mine'].is_producing_resource()
   state['checks'].append('Removing drones stops drill, fan and particles; depleted sources cannot restart production; previews have no VFX');finish(True);return
  state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
