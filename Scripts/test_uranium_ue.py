import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
exec(compile((root/'Scripts/import_uranium_ue.py').read_text(),'import_uranium_ue.py','exec'))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
state={'stage':0,'since':time.monotonic(),'started':time.monotonic(),'checks':[]}
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);spawner=unreal.get_default_object(unreal.GameplayStatics)
def actors(world,cls):return unreal.GameplayStatics.get_all_actors_of_class(world,cls)
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc);a=spawner.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return spawner.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def report(passed,error=None):
 (root/'Saved/UraniumReview.json').write_text(json.dumps({'passed':passed,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
def shot(world):unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
def capture(name):
 files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def tick(dt):
 if state['stage']>0 and time.monotonic()-state['started']>120:report(False,'Timeout');return
 if 1<=state['stage']<=4 and 'weather' in state:
  state['weather'].update_presentation(state['day'] if state['stage']<=2 else state['night'])
 if time.monotonic()-state['since']<5:return
 try:
  world=editor.get_game_world()
  if not world:return
  if state['stage']==0:
   state['started']=time.monotonic();pop=actors(world,unreal.SectorPopulation)[0];grid=actors(world,unreal.HexSectorGrid)[0];weather=actors(world,unreal.PlanetWeatherManager)[0]
   assert 'Borealis' in weather.get_planet_definition().get_name()
   deposits=[d for d in pop.resources.deposits if d.resource_type==unreal.ResourceType.URANIUM_ORE]
   assert any(d.sector_id==grid.starting_sector_id for d in deposits),str(pop.diagnostics)
   assert len(deposits)>1,len(deposits)
   state['checks'].append('Borealis uranium sectors: '+str([d.sector_id for d in deposits]))
   assert all(3000<=d.quantity<=5000 for d in deposits)
   sources=actors(world,unreal.UraniumResourceSource);assert sources,'Start-sector uranium must spawn'
   deposit=next(a for a in sources if grid.get_sector_at_world_location(a.get_actor_location())==grid.starting_sector_id);assert deposit.get_remaining_amount()>=3000
   before=len(sources);pop.reveal_sector(grid.starting_sector_id);assert len(actors(world,unreal.UraniumResourceSource))==before
   assert deposit.get_mine_blueprint() and isinstance(unreal.get_default_object(deposit.get_mine_blueprint()),unreal.MiningMachine)
   mine=unreal.get_default_object(deposit.get_mine_blueprint());assert mine.can_mine_resource_source(deposit),'Start deposit must permit mine construction'
   ordinary=next(a for a in actors(world,unreal.BaseResourceSource) if a.get_resource_type()!=unreal.ResourceType.URANIUM_ORE)
   assert not mine.can_mine_resource_source(ordinary),'Uranium mine must reject ordinary ore'
   assert mine.get_drone_work_type()==unreal.STPDroneWorkType.MINING and mine.get_max_drone_slots()>0
   state['checks'].append('Start uranium is buildable; mine accepts only uranium and keeps drone allocation')
   assert deposit.extract_resource(3)==3
   state['checks'].append('Start deposit has finite reserves, uranium-only mine and no duplicate reveal')
   state['deposit']=deposit
   base=next(a for a in actors(world,unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   fixtures=[]
   for path,offset in zip(['/Game/Data/Buildings/STP/DA_STP_NuclearReactor','/Game/Data/Buildings/BlueprintCatalog/DA_UraniumProcessor'],[unreal.Vector(-650,0,0),unreal.Vector(0,-650,0)]):
    data=unreal.load_asset(path);pos=base.get_actor_location()+offset;hit=unreal.SystemLibrary.line_trace_single(world,pos+unreal.Vector(0,0,1000),pos-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,actors(world,unreal.BaseBuilding),unreal.DrawDebugTrace.NONE);assert hit
    ground=spawner.call_method('BreakHitResult',(hit,))[5];pos.z=ground.z+186;a=spawn(world,data.building_class,pos);a.set_construction_progress(1);fixtures.append(a)
   state['fixtures']=fixtures
   fan=next(c for c in fixtures[1].get_components_by_class(unreal.StaticMeshComponent) if c.get_name().startswith('Motion'));state['fan']=fan;state['rotation']=str(fan.get_editor_property('relative_rotation'))
   pawn=unreal.GameplayStatics.get_player_pawn(world,0);pawn.set_actor_location((base.get_actor_location()+deposit.get_actor_location())*.5+unreal.Vector(-1100,-300,0),False,False)
   weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=12,sun_percent=85));minutes=int(weather.get_planet_definition().get_day_length_minutes());state['weather']=weather;state['night']=min(range(minutes),key=weather.get_planet_definition().get_solar_elevation);state['day']=max(range(minutes),key=weather.get_planet_definition().get_solar_elevation);weather.update_presentation(state['day'])
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,1);state['stage']=1
  elif state['stage']==1:
   assert state['fixtures'][0].is_operational() and state['fixtures'][0].get_energy_production_per_minute()==300
   for a in state['fixtures']+[state['deposit']]:
    effects=a.get_components_by_class(unreal.IndustrialEffectsComponent);assert len(effects)==1
    particles=a.get_components_by_class(unreal.NiagaraComponent);assert particles
    assert any(c.is_active() for c in particles),(a.get_name(),'No active VFX')
   if state['fixtures'][1].is_operational():assert str(state['fan'].get_editor_property('relative_rotation'))!=state['rotation']
   state['checks'].append('Reactor power unchanged; pooled particles and operational fan run')
   shot(world);state['stage']=2
  elif state['stage']==2:capture('UraniumDay.png');state['weather'].update_presentation(state['night']);state['stage']=3
  elif state['stage']==3:shot(world);state['stage']=4
  elif state['stage']==4:
   capture('UraniumNight.png');unreal.GameplayStatics.set_game_paused(world,False);state['fixtures'][0].set_construction_progress(0);state['deposit'].extract_resource(state['deposit'].get_remaining_amount());state['stage']=5
  else:
   for a in [state['fixtures'][0],state['deposit']]:
    status={'actor':a.get_name(),'active':[c.is_active() for c in a.get_components_by_class(unreal.NiagaraComponent)],'visible':[c.is_visible() for c in a.get_components_by_class(unreal.NiagaraComponent)],'paused':unreal.GameplayStatics.is_game_paused(world),'tick':[c.is_component_tick_enabled() for c in a.get_components_by_class(unreal.IndustrialEffectsComponent)]}
    assert not any(status['active']),str(status)
   state['checks'].append('Idle reactor and depleted deposit particles stop; day/night screenshots saved');report(True);return
  state['since']=time.monotonic()
 except Exception:report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
