import unreal,time,json,traceback
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_Borealis')
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
statics=unreal.get_default_object(unreal.GameplayStatics)
state={'stage':0,'since':time.monotonic(),'checks':[]}
def spawn(world,cls,loc):
 t=unreal.Transform(location=loc)
 a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
def tick(dt):
 if time.monotonic()-state['since']<3:return
 world=editor.get_game_world()
 if not world:return
 try:
  if state['stage']==0:
   base=next(b for b in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
   p=spawn(world,unreal.load_asset('/Game/BluePrints/Buildings/STP/BP_STP_NuclearReactor').generated_class(),base.get_actor_location()+unreal.Vector(-650,0,0));p.set_construction_progress(1)
   manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
   manager.add_resource(unreal.ResourceType.FUEL_RODS,100);manager.add_resource(unreal.ResourceType.COOLING_UNITS,100)
   d=spawn(world,unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class(),p.get_actor_location()+unreal.Vector(200,0,180));assert p.try_assign_drone(d,0)
   lib=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
   widgets=lib.call_method('GetAllWidgetsOfClass',(world,unreal.BuildingInfoWidget,False))
   if widgets:
    info=widgets[0];info.set_building(p);state['info']=info
   state.update(p=p,manager=manager,rods=manager.get_resource_amount(unreal.ResourceType.FUEL_RODS),cool=manager.get_resource_amount(unreal.ResourceType.COOLING_UNITS))
   unreal.GameplayStatics.set_game_paused(world,False)
  elif state['stage']==1:
   assert state['p'].is_producing()
   texts={t.get_name():str(t.get_text()) for t in unreal.ObjectIterator(unreal.TextBlock) if t.get_world()==world}
   state['texts']={k:v for k,v in texts.items() if 'Fuel' in k or 'Cooling' in k or k=='CycleText'}
   efficiency=state['p'].get_combined_drone_efficiency()
   expected=f'-{efficiency:.1f}/min'
   assert texts['FuelRodsRateText']==expected,texts
   assert texts['CoolingUnitsRateText']==expected,texts
   if 'info' in state:assert str(state['info'].get_editor_property('cycle_text').get_text())==f'{60/efficiency:.0f} s'
   state['checks'].append(f'One worker at {efficiency}: both inputs show {expected}; cycle {60/efficiency:.0f} s')
   unreal.GameplayStatics.set_global_time_dilation(world,20)
  else:
   used=state['rods']-state['manager'].get_resource_amount(unreal.ResourceType.FUEL_RODS)
   if used==0:return
   assert used==state['cool']-state['manager'].get_resource_amount(unreal.ResourceType.COOLING_UNITS)
   state['checks'].append('Completed cycle consumes both inputs equally')
   unreal.GameplayStatics.set_global_time_dilation(world,1)
   finish(True);return
  state['stage']+=1;state['since']=time.monotonic()
 except Exception:finish(False,traceback.format_exc())
def finish(ok,error=None):
 (root/'Saved/ReactorRatesReview.json').write_text(json.dumps({'passed':ok,'checks':state['checks'],'texts':state.get('texts'), 'error':error},indent=2))
 unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
