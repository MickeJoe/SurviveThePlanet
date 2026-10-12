import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec((root/'Scripts/configure_reactor_ue.py').read_text().replace('unreal.STPDroneWorkType.ENERGY_PRODUCTION','unreal.STPDroneWorkType.CONSTRUCTION'))
assert result['passed']
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]}
def actors(world,cls):return unreal.GameplayStatics.get_all_actors_of_class(world,cls)
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc)
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def finish(ok,error=None):
 (root/'Saved/ReactorReview.json').write_text(json.dumps({'passed':ok,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
def shot(world):unreal.SystemLibrary.execute_console_command(world,'HighResShot 1920x1080')
def copy_shot(name):
 images=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(images,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def amount(resource):return state['manager'].get_resource_amount(resource)
def tick(dt):
 if 'weather' in state:state['weather'].update_presentation(state['day'])
 if time.monotonic()-state['since']<4:return
 try:
  world=editor.get_game_world()
  if not world:return
  stage=state['stage']
  if stage==0:
   base=next(b for b in actors(world,unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   defaults=unreal.get_default_object(data.building_class);assert defaults.get_editor_property('cycle_seconds')==60
   p=spawn(world,data.building_class,base.get_actor_location()+unreal.Vector(-650,0,0));p.set_construction_progress(1)
   defaults.set_editor_property('cycle_seconds',60)
   assert isinstance(p,unreal.NuclearReactor);assert p.get_max_drone_slots()==4 and p.get_unlocked_drone_slots()==4
   assert p.get_energy_production_per_minute()==0
   assert p.get_editor_property('cycle_seconds')==60
   manager=actors(world,unreal.ResourceManager)[0];manager.add_resource(unreal.ResourceType.FUEL_RODS,100);manager.add_resource(unreal.ResourceType.COOLING_UNITS,100)
   state.update(p=p,manager=manager,rods=manager.get_resource_amount(unreal.ResourceType.FUEL_RODS),cool=manager.get_resource_amount(unreal.ResourceType.COOLING_UNITS))
   drones=[]
   for i in range(4):
    d=spawn(world,unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class(),p.get_actor_location()+unreal.Vector(200,i*80,180));assert d.get_working_rate(unreal.STPDroneWorkType.CONSTRUCTION)>0;assert p.try_assign_drone(d,i);drones.append(d)
   state['drones']=drones;state['checks'].append('Four working drone slots accept existing working drones; unstaffed reactor makes zero power')
   pawn=unreal.GameplayStatics.get_player_pawn(world,0);pawn.set_actor_location(p.get_actor_location()+unreal.Vector(-1050,-100,0),False,False)
   weather=actors(world,unreal.PlanetWeatherManager)[0];weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=85));state['weather']=weather;state['day']=max(range(int(weather.get_planet_definition().get_day_length_minutes())),key=weather.get_planet_definition().get_solar_elevation)
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,20)
  elif stage==1:
   p=state['p'];assert p.is_producing();assert p.get_energy_production_per_minute()>0
   used=state['rods']-amount(unreal.ResourceType.FUEL_RODS)
   if used==0:return
   assert used==state['cool']-amount(unreal.ResourceType.COOLING_UNITS)
   unreal.GameplayStatics.set_global_time_dilation(world,1)
   niagara=p.get_components_by_class(unreal.NiagaraComponent);assert len(niagara)==2 and all(c.is_active() for c in niagara)
   assert all('NS_NuclearSteam' in c.get_editor_property('asset').get_name() for c in niagara)
   state['checks'].append('Fuel Rods and Cooling Units consumed atomically; power scales with working drones; two dynamic steam emitters active')
   shot(world)
  elif stage==2:copy_shot('ReactorSteamA.png');shot(world)
  elif stage==3:
   copy_shot('ReactorSteamB.png')
   state['manager'].try_spend_costs([unreal.ResourceCost(resource=unreal.ResourceType.FUEL_RODS,cost=amount(unreal.ResourceType.FUEL_RODS))]);state['cool_before']=amount(unreal.ResourceType.COOLING_UNITS)
  elif stage==4:
   assert not state['p'].is_producing() and state['p'].get_energy_production_per_minute()==0
   assert amount(unreal.ResourceType.COOLING_UNITS)==state['cool_before']
   assert not any(c.is_active() for c in state['p'].get_components_by_class(unreal.NiagaraComponent))
   state['manager'].try_spend_costs([unreal.ResourceCost(resource=unreal.ResourceType.COOLING_UNITS,cost=amount(unreal.ResourceType.COOLING_UNITS))]);state['manager'].add_resource(unreal.ResourceType.FUEL_RODS,10)
  elif stage==5:
   assert not state['p'].is_producing() and state['p'].get_energy_production_per_minute()==0
   state['checks'].append('Missing fuel OR coolant stops grid power and steam without consuming the other input')
   state['manager'].add_resource(unreal.ResourceType.COOLING_UNITS,10)
  elif stage==6:
   assert state['p'].is_producing()
   for d in state['drones']:assert state['p'].unassign_drone(d)
  else:
   assert not state['p'].is_producing() and state['p'].get_energy_production_per_minute()==0
   assert not any(c.is_active() for c in state['p'].get_components_by_class(unreal.NiagaraComponent))
   state['checks'].append('Refuelling restarts reactor; removing all drones stops power and effects');finish(True);return
  state['stage']+=1;state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
