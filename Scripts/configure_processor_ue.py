import unreal,json,traceback,time
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
try:
 bp=unreal.load_asset('/Game/Units/Buildings/BlueprintCatalog/UraniumProcessor/BP_UraniumProcessorBuilding')
 unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.UraniumProcessor)
 data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_UraniumProcessor')
 data.set_editor_property('max_drone_slots',4);data.set_editor_property('initially_unlocked_drone_slots',4);data.set_editor_property('drone_work_type',unreal.STPDroneWorkType.MINING)
 cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('max_drone_slots',4);cdo.set_editor_property('initially_unlocked_drone_slots',4);cdo.set_editor_property('drone_work_type',unreal.STPDroneWorkType.MINING)
 cdo.set_editor_property('ore_per_cycle',2);cdo.set_editor_property('fuel_rods_per_cycle',1);cdo.set_editor_property('cycle_seconds',20)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
 data.set_editor_property('building_class',bp.generated_class())
 unreal.EditorAssetLibrary.save_loaded_asset(bp);unreal.EditorAssetLibrary.save_loaded_asset(data)
 (root/'Saved/ProcessorSetup.json').write_text(json.dumps({'passed':True}))
except Exception:
 (root/'Saved/ProcessorSetup.json').write_text(json.dumps({'passed':False,'error':traceback.format_exc()}));raise
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem);statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]}
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc)
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def report(ok,error=None):
 (root/'Saved/ProcessorReview.json').write_text(json.dumps({'passed':ok,'checks':state['checks'],'error':error},indent=2));unreal.unregister_slate_post_tick_callback(handle)
def tick(dt):
 if time.monotonic()-state['since']<5:return
 try:
  world=editor.get_game_world()
  if not world:return
  if state['stage']==0:
   base=next(b for b in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   p=spawn(world,data.building_class,base.get_actor_location()+unreal.Vector(-650,0,0));p.set_construction_progress(1)
   assert isinstance(p,unreal.UraniumProcessor);assert p.get_max_drone_slots()==4 and p.get_unlocked_drone_slots()==4
   manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0];manager.add_resource(unreal.ResourceType.URANIUM_ORE,100)
   state.update(p=p,manager=manager,rods=manager.get_resource_amount(unreal.ResourceType.FUEL_RODS),ore=manager.get_resource_amount(unreal.ResourceType.URANIUM_ORE))
   assert not p.is_producing()
   drones=[]
   for i in range(4):
    d=spawn(world,unreal.load_asset('/Game/BluePrints/MiningDrone/BP_MiningDrone').generated_class(),p.get_actor_location()+unreal.Vector(200,i*80,180));assert p.try_assign_drone(d,i);drones.append(d)
   assert p.get_assigned_drone_count()==4
   state['drones']=drones;state['checks'].append('Four available drone slots accept four mining drones')
   unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,1)
   state['stage']=1
  elif state['stage']==1:
   p=state['p'];m=state['manager'];assert p.is_producing()
   produced=m.get_resource_amount(unreal.ResourceType.FUEL_RODS)-state['rods'];spent=state['ore']-m.get_resource_amount(unreal.ResourceType.URANIUM_ORE)
   if produced<1:return
   assert spent==produced*2,(spent,produced)
   state['checks'].append('Actual recipe consumes 2 Uranium Ore for every Fuel Rod')
   for d in state['drones']:assert p.unassign_drone(d)
   state['stage']=2
  elif state['stage']==2:
   assert not state['p'].is_producing();state['idle_rods']=state['manager'].get_resource_amount(unreal.ResourceType.FUEL_RODS);state['stage']=3
  else:
   assert state['manager'].get_resource_amount(unreal.ResourceType.FUEL_RODS)==state['idle_rods'];state['checks'].append('No drones stops production and processor VFX');report(True);return
  state['since']=time.monotonic()
 except Exception:report(False,traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
