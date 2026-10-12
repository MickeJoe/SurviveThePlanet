"""Review imported art in PIE and verify grounding, power and existing fan motion."""
import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Aridus')
state={'stage':0,'since':time.monotonic(),'started':time.monotonic(),'checks':[]}
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);spawner=unreal.get_default_object(unreal.GameplayStatics)
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc)
 a=spawner.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return spawner.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def report(passed,error=None):
 (root/'Saved/NuclearIndustrialReview.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2))
 unreal.unregister_slate_post_tick_callback(handle)
def shot(world):unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
def tick(dt):
 if state['stage']>0 and time.monotonic()-state['started']>120:report(False,'Timeout');return
 if time.monotonic()-state['since']<5:return
 try:
  world=editor.get_game_world()
  if not world:return
  if state['stage']==0:
   state['started']=time.monotonic()
   surface=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetSurfaceManager)[0]
   buildings=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding)
   base=next(b for b in buildings if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   data_paths=['/Game/Data/Buildings/STP/DA_STP_NuclearReactor','/Game/Data/Buildings/BlueprintCatalog/DA_UraniumProcessor']
   actors=[]
   for path,offset in zip(data_paths,[unreal.Vector(-650,0,0),unreal.Vector(0,-650,0)]):
    data=unreal.load_asset(path);cls=data.get_editor_property('building_class');cdo=unreal.get_default_object(cls)
    footprint=cdo.get_grid_footprint()
    # Art-review fixtures use open terrain without reserving gameplay cells.
    position=base.get_actor_location()+offset
    hit=unreal.SystemLibrary.line_trace_single(world,position+unreal.Vector(0,0,1000),position-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,buildings+actors,unreal.DrawDebugTrace.NONE)
    assert hit
    ground=spawner.call_method('BreakHitResult',(hit,))[5]
    position.z=ground.z+186
    actor=spawn(world,cls,position);actor.set_construction_progress(1);actors.append(actor)
    assert actor.get_editor_property('ground_mesh_to_surface')
    assert actor.get_building_data()==data
   state['actors']=actors
   pc=unreal.GameplayStatics.get_player_controller(world,0);pawn=unreal.GameplayStatics.get_player_pawn(world,0)
   center=base.get_actor_location()+unreal.Vector(-300,-300,0)
   pawn.set_actor_location(center+unreal.Vector(-1050,-100,0),False,False)
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,1)
   weather=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetWeatherManager)[0]
   weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=12,sun_percent=85))
   minutes=int(weather.get_planet_definition().get_day_length_minutes())
   state['weather']=weather;state['night']=min(range(minutes),key=weather.get_planet_definition().get_solar_elevation)
   weather.update_presentation(max(range(minutes),key=weather.get_planet_definition().get_solar_elevation))
   processor=actors[1]
   fan=next(c for c in processor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name().startswith('Motion'))
   state['fan']=fan;state['rotation']=str(fan.get_editor_property('relative_rotation'))
   state['checks'].append('Both refined Blueprint classes spawn with existing building definitions')
   state['stage']=1
  elif state['stage']==1:
   for actor in state['actors']:
    body=next(c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
    mesh=body.get_editor_property('static_mesh');local=body.get_editor_property('relative_location')
    assert mesh and local.z < -20,(actor.get_name(),str(local))
    pos=actor.get_actor_location();hit=unreal.SystemLibrary.line_trace_single(world,pos+unreal.Vector(0,0,500),pos-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,state['actors'],unreal.DrawDebugTrace.NONE)
    assert hit,'Terrain trace must hit beneath building'
    ground=spawner.call_method('BreakHitResult',(hit,))[5]
    bottom=body.get_world_location().z+mesh.get_bounding_box().min.z*body.get_world_scale().z
    error=bottom-ground.z
    assert abs(error)<1,(actor.get_name(),error)
    state['checks'].append(actor.get_name()+': foundation terrain error '+str(error)+' cm')
   assert state['actors'][0].is_operational(),'Reactor keeps existing sector power connection'
   assert state['actors'][0].get_energy_production_per_minute()==300
   assert str(state['fan'].get_editor_property('relative_rotation'))!=state['rotation'],'Existing processor fan animation advances'
   state['checks'].append('Reactor still produces 300/min; processor roof fan animation preserved')
   shot(world);state['stage']=2
  elif state['stage']==2:
   files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved/NuclearIndustrialDay.png')
   state['weather'].update_presentation(state['night']);state['stage']=3
  elif state['stage']==3:shot(world);state['stage']=4
  else:
   files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved/NuclearIndustrialNight.png');report(True);return
  state['since']=time.monotonic()
 except Exception:report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
