import unreal,json,traceback
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
import time
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn_energy_actor(cls,pos):
 t=unreal.Transform(location=pos);a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
grid=actors(unreal.CableNetworkManager)[0];grid.refresh_energy_grid();other_demand=grid.get_grid_consumption_per_minute()
base=next(a for a in actors(unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
surface=actors(unreal.PlanetSurfaceManager)[0]
data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop');cls=data.get_editor_property('building_class')
placement=None
for dist in [850,1100,1400]:
 for direction in [unreal.Vector(0,1,0),unreal.Vector(1,0,0),unreal.Vector(0,-1,0),unreal.Vector(-1,0,0)]:
  candidate=surface.get_building_placement_for_world_location(base.get_actor_location()+direction*dist,unreal.get_default_object(cls).get_grid_footprint())
  if candidate.valid:placement=candidate;break
 if placement:break
assert placement
plant=spawn_energy_actor(cls,placement.world_location);plant.set_construction_progress(1)
assert surface.reserve_cells(plant,placement.origin_cell,plant.get_grid_footprint())
grid.refresh_energy_grid();assert plant.is_connected_to_power_grid()
base_demand=plant.get_energy_consumption_per_minute();assert base_demand>0
pc=actors(unreal.SurviveThePlanetPlayerController)[0];pc.set_selected_actor(plant)
wl=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
info=wl.call_method('GetAllWidgetsOfClass',(world,unreal.BuildingInfoWidget,False))[0]
drones=actors(unreal.BaseDrone);assert len(drones)==3
drones.append(spawn_energy_actor(unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class(),base.get_actor_location()+unreal.Vector(600,600,150)))
assert all(d.is_available_for_assignment() for d in drones)
energy_state={'index':0,'since':time.monotonic(),'checks':[]}
def energy_tick(dt):
 if time.monotonic()-energy_state['since']<0.5:return
 try:
  i=energy_state['index']
  if i>0:
   multiplier=[1,1.8,2.4,2.8][i-1];expected=base_demand*multiplier
   grid.refresh_energy_grid()
   assert abs(plant.get_energy_consumption_per_minute()-expected)<.001
   assert abs(grid.get_grid_consumption_per_minute()-(other_demand+expected))<.001,(grid.get_grid_consumption_per_minute(),other_demand,expected)
   label=next(o for o in unreal.ObjectIterator(unreal.TextBlock) if o.get_name()=='ConsumptionText' and o.get_path_name().startswith(info.get_path_name()))
   assert str(label.get_text())==f'-{expected:.1f} /min',(str(label.get_text()),expected)
   energy_state['checks'].append({'drones':i,'multiplier':multiplier,'consumption':plant.get_energy_consumption_per_minute(),'ui':str(label.get_text())})
  if i==4:
   plant.unassign_drone(drones[-1]);grid.refresh_energy_grid()
   assert abs(plant.get_energy_consumption_per_minute()-base_demand*2.4)<.001
   plant.set_production_enabled(False)
   assert abs(plant.get_energy_consumption_per_minute()-base_demand*2.4)<.001
   (taskdir/'drone_energy_ui_test.json').write_text(json.dumps({'passed':True,'checks':energy_state['checks'],'removal_updates_demand':True,'existing_paused_demand_preserved':True,'mixed_drone_types_counted':True},indent=2))
   unreal.unregister_slate_post_tick_callback(energy_ui_handle);return
  assert plant.try_assign_drone(drones[i],i)
  energy_state.update(index=i+1,since=time.monotonic())
 except:
  (taskdir/'drone_energy_ui_error.txt').write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(energy_ui_handle)
energy_ui_handle=unreal.register_slate_post_tick_callback(energy_tick)
