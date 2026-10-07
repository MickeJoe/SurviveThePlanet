import unreal,json,time
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
manager=actors(unreal.ResourceManager)[0];grid=actors(unreal.CableNetworkManager)[0];controller=actors(unreal.SurviveThePlanetPlayerController)[0]
camp=[a for a in actors(unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE][0]
data=unreal.load_asset("/Game/Data/Buildings/DA_ConnectorPlant")
assert data in list(unreal.load_asset("/Game/Data/Buildings/DA_BuildingCatalog").get_editor_property("buildings"))
assert data.get_editor_property("energy_consumption_per_minute")==12
assert (data.get_editor_property("copper_per_cycle"),data.get_editor_property("polymer_per_cycle"),data.get_editor_property("connectors_per_cycle"),data.get_editor_property("cycle_seconds"))==(2,1,3,20)
cls=data.get_editor_property("building_class");assert cls
statics=unreal.get_default_object(unreal.GameplayStatics)
library=unreal.get_default_object(unreal.load_class(None,"/Script/UMG.WidgetBlueprintLibrary"))
toolbar=library.call_method("GetAllWidgetsOfClass",(world,unreal.BuildToolbarWidget,False))[0]
display=library.call_method("GetAllWidgetsOfClass",(world,unreal.ResourceDisplayWidget,False))[0]
for key in ["connector_icon","connector_amount_text","connector_rate_text"]:assert display.get_editor_property(key),key
for res,n in [(unreal.ResourceType.IRON,100),(unreal.ResourceType.CONCRETE,100),(unreal.ResourceType.CONTROL_CHIP,10),(unreal.ResourceType.COPPER,100),(unreal.ResourceType.POLYMER,100),(unreal.ResourceType.CONNECTOR,0),(unreal.ResourceType.ENERGY,1000)]:
 manager.set_resource_amount(res,n)
toolbar.call_method("HandleIndustryCategoryClicked",())
bindings=[b for b in unreal.ObjectIterator(unreal.load_class(None,"/Script/SurviveThePlanet.BuildToolbarClickBinding")) if b.get_outer()==toolbar]
for binding in bindings:
 binding.call_method("Click",())
 if controller.get_active_build_tool()==unreal.STPBuildTool.CONNECTOR_PLANT:break
assert controller.get_active_build_tool()==unreal.STPBuildTool.CONNECTOR_PLANT,"Missing live Industry button"
surface=actors(unreal.PlanetSurfaceManager)[0];footprint=unreal.get_default_object(cls).get_grid_footprint();placement=None
for distance in [850,1100,1400,1800,2200]:
 for direction in [unreal.Vector(0,1,0),unreal.Vector(1,0,0),unreal.Vector(0,-1,0),unreal.Vector(-1,0,0)]:
  candidate=surface.get_building_placement_for_world_location(camp.get_actor_location()+direction*distance,footprint)
  if candidate.valid:placement=candidate;break
 if placement:break
assert placement
location=placement.world_location;transform=unreal.Transform(location=location,rotation=placement.world_rotation)
plant=statics.call_method("BeginDeferredActorSpawnFromClass",(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
statics.call_method("FinishSpawningActor",(plant,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
assert isinstance(plant,unreal.ConnectorPlant)
assert surface.reserve_cells(plant,placement.origin_cell,plant.get_grid_footprint())
assert manager.try_spend_costs(list(data.get_editor_property("construction_costs")))
mesh=next(c for c in plant.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=="BuildingMesh")
assert mesh.static_mesh
hit=unreal.SystemLibrary.line_trace_single(world,location+unreal.Vector(0,0,500),location-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,actors(unreal.BaseBuilding),unreal.DrawDebugTrace.NONE,True)
fields=statics.call_method("BreakHitResult",(hit,));assert fields[0]
assert abs(mesh.get_world_location().z+mesh.static_mesh.get_bounding_box().min.z-fields[5].z)<1
assert plant.get_construction_progress()<1
light=plant.get_components_by_class(unreal.PointLightComponent)[0]
unreal.GameplayStatics.set_game_paused(world,False);unreal.GameplayStatics.set_global_time_dilation(world,10)
fan=plant.get_editor_property("fan_a");products=list(plant.get_editor_property("conveyor_products"));sparks=list(plant.get_editor_property("welding_sparks"));assert fan.static_mesh and len(products)==3 and all(p.static_mesh for p in products)
fan_before=str(fan.get_editor_property("relative_rotation"));product_before=str(products[0].get_editor_property("relative_location"));seen_weld=False
phase=0;started=time.monotonic();report={"passed":False,"checks":["recipe","Industry button click selects Connector Plant","HUD bindings","valid reserved grid footprint","construction cost payment","terrain contact"],"footprint":str(footprint)}
def finish():
 (root/"Saved/ConnectorPlantRuntimeTest.json").write_text(json.dumps(report,indent=2))
 unreal.unregister_slate_post_tick_callback(handle)
 unreal.GameplayStatics.set_global_time_dilation(world,1)
 controller.set_active_build_tool(unreal.STPBuildTool.NONE)
 unreal.log("CONNECTOR_PLANT_TEST "+json.dumps(report))
def check(delta):
 global phase,started,produced,paused_output,energy_before,seen_weld,stopped_fan,stopped_product
 seen_weld=seen_weld or (plant.is_producing() and light.get_editor_property("intensity")>0 and any(p.is_visible() for p in sparks))
 if time.monotonic()-started<(2.4 if phase==1 else .45):return
 try:
  if phase==0:
   previews=[a for a in actors(unreal.ConnectorPlant) if a.is_placement_preview()]
   assert previews and not previews[0].is_operational()
   assert not plant.is_producing() and manager.get_resource_amount(unreal.ResourceType.CONNECTOR)==0
   controller.set_active_build_tool(unreal.STPBuildTool.NONE)
   plant.set_construction_progress(1);grid.refresh_energy_grid()
   assert plant.is_connected_to_power_grid() and plant.is_operational()
   energy_before=manager.get_resource_amount(unreal.ResourceType.ENERGY)
  elif phase==1:
   produced=manager.get_resource_amount(unreal.ResourceType.CONNECTOR);assert produced>=3 and produced%3==0
   cycles=produced//3
   assert manager.get_resource_amount(unreal.ResourceType.COPPER)==100-2*cycles
   assert manager.get_resource_amount(unreal.ResourceType.POLYMER)==100-cycles
   assert manager.get_resource_amount(unreal.ResourceType.ENERGY)<energy_before
   assert plant.get_connector_production_per_minute()==9 and plant.get_copper_consumption_per_minute()==6 and plant.get_polymer_consumption_per_minute()==3
   assert seen_weld,"No welding burst visible"
   assert str(fan.get_editor_property("relative_rotation"))!=fan_before,"Fan static"
   assert str(products[0].get_editor_property("relative_location"))!=product_before,"Belt products static"
   assert str(display.get_editor_property("connector_amount_text").get_text())==str(produced)
   assert str(display.get_editor_property("connector_rate_text").get_text())=="+9.0/min"
   report["produced"]=produced;report["checks"]+=["preview and incomplete construction cannot produce","powered production","copper/polymer conservation","energy consumption","live HUD amount and rate","rotating fans","moving belt products","welding sparks and pulsing light"]
   manager.set_resource_amount(unreal.ResourceType.COPPER,0);paused_output=produced
  elif phase==2:
   assert not plant.is_producing() and manager.get_resource_amount(unreal.ResourceType.CONNECTOR)==paused_output
   assert light.get_editor_property("intensity")==0 and not any(p.is_visible() for p in sparks)
   stopped_fan=str(fan.get_editor_property("relative_rotation"));stopped_product=str(products[0].get_editor_property("relative_location"))
   manager.set_resource_amount(unreal.ResourceType.COPPER,100);manager.set_resource_amount(unreal.ResourceType.POLYMER,0)
  elif phase==3:
   assert str(fan.get_editor_property("relative_rotation"))==stopped_fan and str(products[0].get_editor_property("relative_location"))==stopped_product,"Animation continued while starved"
   assert not plant.is_producing() and manager.get_resource_amount(unreal.ResourceType.CONNECTOR)==paused_output
   manager.set_resource_amount(unreal.ResourceType.POLYMER,100);manager.set_resource_amount(unreal.ResourceType.ENERGY,0);grid.refresh_energy_grid()
  elif phase==4:
   assert not plant.is_operational() and not plant.is_producing()
   assert manager.get_resource_amount(unreal.ResourceType.CONNECTOR)==paused_output
   manager.set_resource_amount(unreal.ResourceType.ENERGY,1000)
   plant.set_actor_location(camp.get_actor_location()+unreal.Vector(30000,0,0),False,False);grid.refresh_energy_grid()
  elif phase==5:
   assert not plant.is_operational() and not plant.is_producing()
   plant.set_actor_location(location,False,False);grid.refresh_energy_grid()
  else:
   assert plant.is_producing()
   report["checks"]+=["copper starvation stops animation and VFX","polymer starvation stops","empty energy stops","outside coverage stops","restart resumes"]
   report["passed"]=True;finish();return
  phase+=1;started=time.monotonic()
 except Exception as error:
  report["error"]=repr(error);finish();unreal.log_error("CONNECTOR_PLANT_TEST_FAILED "+repr(error))
handle=unreal.register_slate_post_tick_callback(check)

