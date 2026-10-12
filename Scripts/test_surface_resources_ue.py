"""Verify actual planet generation, discovery, mine placement and extraction in PIE."""
import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');planet_name=globals().get('SURFACE_TEST_PLANET','Aridus')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
planet=next(unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets('/Game/Data/Planet',True,False) if isinstance(unreal.load_asset(p),unreal.PlanetDefinition) and planet_name in unreal.load_asset(p).get_name())
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_'+planet_name)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
types=[unreal.ResourceType.SILICA,unreal.ResourceType.RARE_MINERALS,unreal.ResourceType.ALIEN_MATERIAL]
state={'stage':0,'since':time.monotonic(),'checks':[],'planet':planet_name}
def actors(w,c):return unreal.GameplayStatics.get_all_actors_of_class(w,c)
def spawn(w,cls,t):
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(w,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def finish(passed,error=None):
 result={'passed':passed,'planet':planet_name,'checks':state['checks'],'deposits':state.get('deposits',[]),'error':error};(root/'Saved'/('SurfaceResources_'+planet_name+'.json')).write_text(json.dumps(result,indent=2));unreal.log('SURFACE_RESOURCE_REVIEW '+json.dumps(result));unreal.unregister_slate_post_tick_callback(handle)
def capture(name):
 files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def tick(dt):
 if state['stage'] and time.monotonic()-state['started']>150:finish(False,'Timeout');return
 if 'weather' in state:state['weather'].update_presentation(state['night'] if state['stage']>=4 else state['day'])
 if time.monotonic()-state['since']<(12 if state['stage']==1 else 4):return
 try:
  w=editor.get_game_world()
  if not w:return
  if state['stage']==0:
   state['started']=time.monotonic();pop=actors(w,unreal.SectorPopulation)[0];grid=actors(w,unreal.HexSectorGrid)[0];weather=actors(w,unreal.PlanetWeatherManager)[0]
   assert weather.get_planet_definition()==planet
   planned=[d for d in pop.resources.deposits if d.resource_type in types];allowed=[unreal.ResourceType.SILICA]
   if planet_name=='Nexaris':allowed.append(unreal.ResourceType.RARE_MINERALS)
   if planet_name=='Aridus':allowed.append(unreal.ResourceType.ALIEN_MATERIAL)
   assert set(d.resource_type for d in planned)==set(allowed),(str(pop.diagnostics),[str(d.resource_type) for d in planned])
   for t in allowed:
    entries=[d for d in planned if d.resource_type==t];assert sum(d.sector_id==grid.starting_sector_id for d in entries)==1,(str(t),str(pop.diagnostics))
    assert len(entries)>1,(str(t),'Must distribute into other sectors')
    assert all(3000<=d.quantity<=5000 for d in entries)
   starts=[s for s in actors(w,unreal.BaseResourceSource) if s.get_resource_type() in types];assert len(starts)==len(allowed)
   assert all(grid.get_sector_at_world_location(s.get_actor_location())==grid.starting_sector_id for s in starts)
   pop.reveal_sector(grid.starting_sector_id);assert len([s for s in actors(w,unreal.BaseResourceSource) if s.get_resource_type() in types])==len(starts)
   state['deposits']=[{'type':str(d.resource_type),'sector':d.sector_id,'quantity':d.quantity,'position':[d.location.x,d.location.y,d.location.z]} for d in planned]
   state['checks'].append('Exactly one allowed deposit per type in start sector; other sectors generated; excluded types absent')
   for d in planned:pop.reveal_sector(d.sector_id)
   sources=[s for s in actors(w,unreal.BaseResourceSource) if s.get_resource_type() in types];assert len(sources)==len(planned)
   for s in sources:
    defaults=unreal.get_default_object(s.get_mine_blueprint());assert isinstance(defaults,unreal.MiningMachine);assert defaults.can_mine_resource_source(s),(s.get_name(),str(s.get_actor_location()))
   state['checks'].append('Every generated deposit accepts its mining building; discovery spawns once')
   if planet_name=='Borealis':
    uranium=[d for d in pop.resources.deposits if d.resource_type==unreal.ResourceType.URANIUM_ORE];assert len(uranium)>1 and sum(d.sector_id==grid.starting_sector_id for d in uranium)==1
    for d in uranium:pop.reveal_sector(d.sector_id)
    for source in actors(w,unreal.UraniumResourceSource):assert unreal.get_default_object(source.get_mine_blueprint()).can_mine_resource_source(source)
    state['checks'].append('Existing Borealis uranium generation and mine clearance remain valid alongside silica')
   # Inspect untouched deposits in the normal gameplay camera before replacing them.
   base=next(a for a in actors(w,unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   center=base.get_actor_location()
   for source in starts:center+=source.get_actor_location()
   center/=(len(starts)+1)
   pawn=unreal.GameplayStatics.get_player_pawn(w,0);pawn.set_actor_location(center,False,False);pawn.set_actor_tick_enabled(False)
   weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=85));minutes=int(planet.get_day_length_minutes())
   state.update(weather=weather,day=max(range(minutes),key=planet.get_solar_elevation),night=min(range(minutes),key=planet.get_solar_elevation),starts=starts)
   unreal.GameplayStatics.set_game_paused(w,False);unreal.GameplayStatics.set_global_time_dilation(w,1);state['stage']=1
  elif state['stage']==1:
   unreal.SystemLibrary.execute_console_command(w,'HighResShot 1920x1080');state['stage']=2
  elif state['stage']==2:
   capture('SurfaceResources_'+planet_name+'_Day.png');state['stage']=4
  elif state['stage']==4:
   unreal.SystemLibrary.execute_console_command(w,'HighResShot 1920x1080');state['stage']=5
  elif state['stage']==5:
   capture('SurfaceResources_'+planet_name+'_Night.png');fixtures=[];manager=actors(w,unreal.ResourceManager)[0]
   droneclass=unreal.load_asset('/Game/BluePrints/MiningDrone/BP_MiningDrone').generated_class()
   for source in state['starts']:
    defaults=unreal.get_default_object(source.get_mine_blueprint())
    for other in state['starts']:
     if other!=source:assert not defaults.can_mine_resource_source(other),'Mine must reject a different resource type'
    mine=spawn(w,source.get_mine_blueprint(),defaults.get_placement_transform_for_source(source));assert mine.attach_to_resource_source(source);mine.set_construction_progress(1)
    assert mine.get_max_drone_slots()==4;mine.set_unlocked_drone_slots(4)
    assert not mine.is_producing_resource(),'Mine must wait for a drone'
    drones=[]
    for i in range(4):
     drone=spawn(w,droneclass,unreal.Transform(location=mine.get_actor_location()+unreal.Vector(160,i*60,180)));assert mine.try_assign_drone(drone,i);drones.append(drone)
    assert mine.get_assigned_drone_count()==4
    assert mine.is_producing_resource(),(mine.get_name(),mine.is_operational(),mine.get_combined_drone_efficiency())
    fixtures.append({'mine':mine,'source':source,'drones':drones,'reserve':source.get_remaining_amount(),'stock':manager.get_resource_amount(source.get_resource_type())})
   state.update(fixtures=fixtures,manager=manager);state['stage']=6
  elif state['stage']==6:
   for f in state['fixtures']:
    source=f['source'];mine=f['mine'];used=f['reserve']-source.get_remaining_amount();gain=state['manager'].get_resource_amount(source.get_resource_type())-f['stock'];assert used>0 and gain==used,(mine.get_name(),used,gain)
    assert abs(mine.get_current_output_per_minute()-10*mine.get_combined_drone_efficiency())<.01
    for drone in f['drones']:assert mine.unassign_drone(drone)
    assert not mine.is_producing_resource();assert mine.get_current_output_per_minute()==0;assert mine.get_energy_consumption_per_minute()==0
    before=source.get_remaining_amount();assert source.extract_resource(before+99)==before;assert source.get_remaining_amount()==0;assert source.extract_resource(1)==0
   state['checks'].append('Four assigned drones extract only the matching finite resource into stock; idle/depleted production stops')
   finish(True);return
  state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
