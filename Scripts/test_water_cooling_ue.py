import unreal,json,time,traceback,shutil
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Aridus')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]}
def actors(w,c):return unreal.GameplayStatics.get_all_actors_of_class(w,c)
def spawn(w,cls,pos,incomplete=False):
 t=unreal.Transform(location=pos)
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(w,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 if incomplete:a.set_construction_progress(0)
 return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def finish(ok,error=None):
 (root/'Saved/WaterCoolingReview.json').write_text(json.dumps({'passed':ok,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
 w=editor.get_game_world()
 if w:unreal.GameplayStatics.set_global_time_dilation(w,1)
def snapshot():
 return {str(r):state['manager'].get_resource_amount(r) for r in [unreal.ResourceType.WATER,unreal.ResourceType.PURIFIED_WATER,unreal.ResourceType.STEEL,unreal.ResourceType.COOLING_UNITS]}
def shot(w):unreal.SystemLibrary.execute_console_command(w,'HighResShot 1920x1080')
def copyshot(name):
 files=list((root/'Saved/Screenshots').rglob('HighresScreenshot*.png'));shutil.copy2(max(files,key=lambda p:p.stat().st_mtime),root/'Saved'/name)
def tick(dt):
 if time.monotonic()-state['since']<3:return
 try:
  w=editor.get_game_world()
  if not w:return
  s=state['stage']
  if s==0:
   base=next(a for a in actors(w,unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   manager=actors(w,unreal.ResourceManager)[0];state['manager']=manager
   for r in [unreal.ResourceType.WATER,unreal.ResourceType.PURIFIED_WATER,unreal.ResourceType.STEEL,unreal.ResourceType.ENERGY]:manager.add_resource(r,10000)
   buildings=[]
   for key,offset in [('WaterPurifier',unreal.Vector(-650,0,0)),('CoolingSystemsPlant',unreal.Vector(0,-650,0))]:
    data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_'+key);a=spawn(w,data.get_editor_property('building_class'),base.get_actor_location()+offset);a.set_construction_progress(1)
    assert isinstance(a,unreal.ThermalProcessingBuilding);assert a.get_max_drone_slots()==4 and a.get_unlocked_drone_slots()==4
    assert not a.is_producing();assert a.get_energy_consumption_per_minute()==12
    buildings.append(a)
   state['buildings']=buildings;state['drone_groups']=[]
   for a in buildings:
    body=next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
    mesh=body.get_editor_property('static_mesh');pos=a.get_actor_location()
    hit=unreal.SystemLibrary.line_trace_single(w,pos+unreal.Vector(0,0,1000),pos-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,actors(w,unreal.BaseBuilding),unreal.DrawDebugTrace.NONE)
    assert hit,'Terrain not found beneath building'
    fields=statics.call_method('BreakHitResult',(hit,));assert fields[0]
    bottom=body.get_world_location().z+mesh.get_bounding_box().min.z*body.get_world_scale().z
    assert abs(bottom-fields[5].z)<1,(a.get_name(),bottom,fields[5].z)
   state['checks'].append('Both refined foundations contact terrain within 1 cm at the gameplay camera')
   cls=unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class()
   for a in buildings:
    drones=[]
    for i in range(4):
     d=spawn(w,cls,a.get_actor_location()+unreal.Vector(150,i*80,180));assert a.try_assign_drone(d,i);drones.append(d)
    extra=spawn(w,cls,a.get_actor_location()+unreal.Vector(200,400,180));assert not a.try_assign_drone(extra);extra.destroy_actor()
    for d in drones:assert a.unassign_drone(d)
    state['drone_groups'].append(drones)
   state['checks'].append('Both catalog Blueprints spawn with four slots and reject a fifth drone; zero drones stops production')
   weather=actors(w,unreal.PlanetWeatherManager)[0];state['weather']=weather
   minutes=int(weather.get_planet_definition().get_day_length_minutes());state['day']=max(range(minutes),key=weather.get_planet_definition().get_solar_elevation);state['night']=min(range(minutes),key=weather.get_planet_definition().get_solar_elevation)
   weather.set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=85));weather.update_presentation(state['day'])
   pawn=unreal.GameplayStatics.get_player_pawn(w,0);pawn.set_actor_location(base.get_actor_location()+unreal.Vector(-1350,-350,0),False,False)
   unreal.GameplayStatics.set_game_paused(w,False);unreal.GameplayStatics.set_global_time_dilation(w,1)
   state['before']=snapshot()
  elif s==1:
   assert snapshot()==state['before'],'Unstaffed buildings consumed resources'
   for a,drones in zip(state['buildings'],state['drone_groups']):
    assert a.is_operational(),'Fixture is not connected to a powered sector'
    first_rate=drones[0].get_working_rate(unreal.STPDroneWorkType.CONSTRUCTION)
    for n,d in enumerate(drones,1):
     assert a.try_assign_drone(d,n-1);assert abs(a.get_combined_drone_efficiency()-sum(x.get_working_rate(unreal.STPDroneWorkType.CONSTRUCTION) for x in drones[:n]))<.01
    fan=next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='MotionFan');assert fan.get_attach_parent().get_name()=='BuildingMesh'
   state['rotations']=[str(next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='MotionFan').get_editor_property('relative_rotation')) for a in state['buildings']]
   state['before']=snapshot();unreal.GameplayStatics.set_global_time_dilation(w,10)
   state['checks'].append('1, 2, 3 and 4 drone efficiencies add linearly; fans attach to grounded building meshes')
  elif s==2:
   now=snapshot();before=state['before'];p,c=state['buildings']
   assert p.is_producing() and c.is_producing()
   clean_created=(before[str(unreal.ResourceType.WATER)]-now[str(unreal.ResourceType.WATER)])/2
   cool_created=now[str(unreal.ResourceType.COOLING_UNITS)]-before[str(unreal.ResourceType.COOLING_UNITS)]
   assert clean_created>0 and cool_created>0
   assert before[str(unreal.ResourceType.STEEL)]-now[str(unreal.ResourceType.STEEL)]==cool_created*2
   assert now[str(unreal.ResourceType.PURIFIED_WATER)]-before[str(unreal.ResourceType.PURIFIED_WATER)]==clean_created-cool_created
   for i,a in enumerate(state['buildings']):
    fan=next(x for x in a.get_components_by_class(unreal.StaticMeshComponent) if x.get_name()=='MotionFan');assert str(fan.get_editor_property('relative_rotation'))!=state['rotations'][i]
    fx=[x for x in a.get_components_by_class(unreal.NiagaraComponent) if x.get_editor_property('asset')];assert len(fx)==1 and fx[0].is_active()
   texts={t.get_name():str(t.get_text()) for t in unreal.ObjectIterator(unreal.TextBlock) if t.get_world()==w}
   pc=60*p.get_combined_drone_efficiency()/20;cc=60*c.get_combined_drone_efficiency()/20
   assert texts['CoolingUnitsRateText']==f'+{cc:.1f}/min',texts.get('CoolingUnitsRateText')
   assert texts['PurifiedWaterRateText']==f'{pc-cc:+.1f}/min',texts.get('PurifiedWaterRateText')
   lib=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'));infos=lib.call_method('GetAllWidgetsOfClass',(w,unreal.BuildingInfoWidget,False));assert infos
   info=infos[0]
   for a,count in [(p,1),(c,3)]:
    info.set_building(a);assert info.get_editor_property('inputs_box').get_children_count()==count
    assert str(info.get_editor_property('cycle_text').get_text())==f'{20/a.get_combined_drone_efficiency():.0f} s'
    assert str(info.get_editor_property('consumption_text').get_text())=='-12.0 /min'
   info.set_visibility(unreal.SlateVisibility.COLLAPSED)
   state['checks'].append('HUD shows net purified water and cooling-unit rates; UMG production panel shows correct inputs, power and drone-scaled cycle')
   state['checks'].append('Running cycles use exactly the documented resource types and configured quantities; fans rotate and one pooled steam emitter per building runs')
   unreal.GameplayStatics.set_global_time_dilation(w,1);shot(w)
  elif s==3:
   copyshot('WaterCoolingDay.png');state['weather'].set_weather_immediately(unreal.PlanetWeatherState(precipitation_percent=0,wind_percent=8,sun_percent=0));state['weather'].update_presentation(state['night']);shot(w)
  elif s==4:
   copyshot('WaterCoolingNight.png')
   for r in [unreal.ResourceType.WATER,unreal.ResourceType.STEEL]:state['manager'].set_resource_amount(r,0)
   state['idle_rotations']=[str(next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='MotionFan').get_editor_property('relative_rotation')) for a in state['buildings']]
  elif s==5:
   for a in state['buildings']:assert not a.is_producing();assert not any(c.is_active() for c in a.get_components_by_class(unreal.NiagaraComponent))
   state['idle_before']=snapshot();state['idle_rotations']=[str(next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='MotionFan').get_editor_property('relative_rotation')) for a in state['buildings']]
  elif s==6:
   assert snapshot()==state['idle_before'],'Missing inputs consumed part of a recipe'
   for i,a in enumerate(state['buildings']):assert str(next(c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='MotionFan').get_editor_property('relative_rotation'))==state['idle_rotations'][i]
   state['checks'].append('Missing inputs stop both production and VFX/fans without consuming partial recipes; day and night screenshots captured')
   state['manager'].set_resource_amount(unreal.ResourceType.STEEL,1000);state['manager'].set_resource_amount(unreal.ResourceType.PURIFIED_WATER,0);state['blocked_before']=snapshot()
  elif s==7:
   assert not state['buildings'][1].is_producing();assert snapshot()==state['blocked_before']
   state['checks'].append('Cooling plant also stops when purified water alone is missing, preserving steel')
   for r in [unreal.ResourceType.WATER,unreal.ResourceType.PURIFIED_WATER]:state['manager'].add_resource(r,1000)
  elif s==8:
   assert all(a.is_producing() for a in state['buildings'])
   for a,drones in zip(state['buildings'],state['drone_groups']):
    for d in drones:assert a.unassign_drone(d)
  elif s==9:
   assert all(not a.is_producing() for a in state['buildings']);assert all(not c.is_active() for a in state['buildings'] for c in a.get_components_by_class(unreal.NiagaraComponent))
   state['checks'].append('Restocking restarts production; removing all drones stops both buildings and effects')
   fixtures=[]
   for a,drones in zip(state['buildings'],state['drone_groups']):
    cls=a.get_class();unpowered=spawn(w,cls,a.get_actor_location()+unreal.Vector(15000,0,0));unpowered.set_construction_progress(1);assert unpowered.try_assign_drone(drones[0]);fixtures.append(unpowered)
    unfinished=spawn(w,cls,a.get_actor_location()+unreal.Vector(0,500,0),True);fixtures.append(unfinished)
   state['fixtures']=fixtures
  else:
   assert all(not a.is_producing() for a in state['fixtures'])
   assert all(not c.is_active() for a in state['fixtures'] for c in a.get_components_by_class(unreal.NiagaraComponent))
   state['checks'].append('Unpowered staffed buildings and unfinished construction cannot produce resources or active effects');finish(True);return
  state['stage']+=1;state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
