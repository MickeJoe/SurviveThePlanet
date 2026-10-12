"""Exercise the actual uranium source/mine construction and drone workflow in PIE."""
import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]}
def actors(world,cls):return unreal.GameplayStatics.get_all_actors_of_class(world,cls)
def spawn(world,cls,transform,preview=False):
 actor=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 if preview:actor.set_placement_preview(True)
 return statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def report(passed,error=None):
 (root/'Saved/UraniumMineReview.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
def shot(world):unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
def capture(name):
 images=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(images,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def tick(dt):
 if state['stage'] and time.monotonic()-state['started']>120:report(False,'Timeout');return
 if 'weather' in state and 1<=state['stage']<=4:state['weather'].update_presentation(state['night'] if state['stage']>=3 else state['day'])
 if time.monotonic()-state['since']<(12 if state['stage']==1 else 5):return
 try:
  world=editor.get_game_world()
  if not world:return
  if state['stage']==0:
   state['started']=time.monotonic();pop=actors(world,unreal.SectorPopulation)[0];grid=actors(world,unreal.HexSectorGrid)[0]
   deposits=[d for d in pop.resources.deposits if d.resource_type==unreal.ResourceType.URANIUM_ORE]
   for d in deposits:pop.reveal_sector(d.sector_id)
   sources=actors(world,unreal.UraniumResourceSource);source=next(s for s in sources if grid.get_sector_at_world_location(s.get_actor_location())==grid.starting_sector_id)
   cls=source.get_mine_blueprint();defaults=unreal.get_default_object(cls)
   assert all(defaults.can_mine_resource_source(s) for s in sources),'Refined mine must fit every generated uranium source'
   state['checks'].append('New mine fits all '+str(len(sources))+' generated uranium deposits')
   transform=defaults.get_placement_transform_for_source(source);preview=spawn(world,cls,transform,True)
   assert not preview.get_components_by_class(unreal.NiagaraComponent),'Preview must create no operating particles'
   preview.destroy_actor()
   mine=spawn(world,cls,transform);assert mine.attach_to_resource_source(source),'Actual mine construction must reserve source cells';mine.set_construction_progress(1)
   assert mine.get_max_drone_slots()==4 and mine.get_unlocked_drone_slots()==1
   assert not mine.is_producing_resource(),'Unstaffed mine must remain idle'
   body=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh');assert 'Refined' in body.get_editor_property('static_mesh').get_name()
   drill=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name().startswith('MineDrill'))
   state.update(mine=mine,source=source,drill=drill,rotation=str(drill.get_editor_property('relative_rotation')),location=str(drill.get_editor_property('relative_location')),amount=source.get_remaining_amount())
   drone=spawn(world,unreal.load_asset('/Game/BluePrints/MiningDrone/BP_MiningDrone').generated_class(),unreal.Transform(location=mine.get_actor_location()+unreal.Vector(200,0,180)))
   assert drone.get_working_rate(unreal.STPDroneWorkType.MINING)>0
   assert mine.try_assign_drone(drone,0);assert mine.is_producing_resource()
   state['drone']=drone;state['checks'].append('Construction, source reservation and existing 4/1 drone slots work')
   base=next(b for b in actors(world,unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   pawn=unreal.GameplayStatics.get_player_pawn(world,0);pawn.set_actor_location((base.get_actor_location()+mine.get_actor_location())*.5+unreal.Vector(-1100,-300,0),False,False)
   weather=actors(world,unreal.PlanetWeatherManager)[0];weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=85))
   minutes=int(weather.get_planet_definition().get_day_length_minutes());state['weather']=weather;state['day']=max(range(minutes),key=weather.get_planet_definition().get_solar_elevation);state['night']=min(range(minutes),key=weather.get_planet_definition().get_solar_elevation)
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,1);state['stage']=1
  elif state['stage']==1:
   mine=state['mine'];drill=state['drill'];assert mine.is_producing_resource();assert str(drill.get_editor_property('relative_rotation'))!=state['rotation'];assert str(drill.get_editor_property('relative_location'))!=state['location']
   assert any(c.is_active() for c in mine.get_components_by_class(unreal.NiagaraComponent))
   assert state['source'].get_remaining_amount()<state['amount'],'Assigned drone must extract actual uranium'
   body=next(c for c in mine.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh');pos=mine.get_actor_location()
   hit=unreal.SystemLibrary.line_trace_single(world,pos+unreal.Vector(0,0,1000),pos-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[mine,state['source'],state['drone']],unreal.DrawDebugTrace.NONE);assert hit
   ground=statics.call_method('BreakHitResult',(hit,))[5];bottom=body.get_world_location().z+body.get_editor_property('static_mesh').get_bounding_box().min.z*body.get_world_scale().z
   assert abs(bottom-ground.z)<1,('Foundation height error',bottom-ground.z)
   state['checks'].append('Foundation rests on terrain: '+str(bottom-ground.z)+' cm error')
   state['checks'].append('Assigned mining drone extracts uranium; drill rotates/feeds and dust runs')
   shot(world);state['stage']=2
  elif state['stage']==2:capture('UraniumMineDay.png');state['stage']=3
  elif state['stage']==3:
   assert any(c.get_editor_property('intensity')>0 for c in state['mine'].get_components_by_class(unreal.PointLightComponent))
   shot(world);state['stage']=4
  elif state['stage']==4:
   capture('UraniumMineNight.png');assert state['mine'].unassign_drone(state['drone']);state['idle_rotation']=str(state['drill'].get_editor_property('relative_rotation'));state['stage']=5
  elif state['stage']==5:
   assert not state['mine'].is_producing_resource();assert not any(c.is_active() for c in state['mine'].get_components_by_class(unreal.NiagaraComponent))
   state['settled_rotation']=str(state['drill'].get_editor_property('relative_rotation'));state['stage']=6
  else:
   assert str(state['drill'].get_editor_property('relative_rotation'))==state['settled_rotation'],'Unstaffed drill must stop'
   state['checks'].append('Night worklights follow darkness; removing drone stops drill and dust');report(True);return
  state['since']=time.monotonic()
 except Exception:report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
