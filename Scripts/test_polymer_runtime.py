import unreal,json,time
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
manager=actors(unreal.ResourceManager)[0];grid=actors(unreal.CableNetworkManager)[0];controller=actors(unreal.SurviveThePlanetPlayerController)[0]
camps=[a for a in actors(unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE];assert camps
camp=camps[0]
catalog=unreal.load_asset("/Game/Data/Buildings/DA_BuildingCatalog")
definition=unreal.load_asset("/Game/Data/Buildings/DA_PolymerPlant")
assert definition in list(catalog.get_editor_property("buildings"))
assert definition.get_editor_property("build_tool")==unreal.STPBuildTool.POLYMER_PLANT
cls=unreal.load_class(None,"/Game/BluePrints/Buildings/PolymerPlant/BP_PolymerPlant.BP_PolymerPlant_C");assert cls
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn(transform,preview=False):
 a=statics.call_method("BeginDeferredActorSpawnFromClass",(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 if preview:a.set_placement_preview(True)
 statics.call_method("FinishSpawningActor",(a,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
 return a
library=unreal.get_default_object(unreal.load_class(None,"/Script/UMG.WidgetBlueprintLibrary"))
toolbar=library.call_method("GetAllWidgetsOfClass",(world,unreal.BuildToolbarWidget,False))[0]
display=library.call_method("GetAllWidgetsOfClass",(world,unreal.ResourceDisplayWidget,False))[0]
assert display.get_editor_property("polymer_icon")
assert display.get_editor_property("polymer_amount_text")
assert display.get_editor_property("polymer_rate_text")
for res,amount in [(unreal.ResourceType.COAL,100),(unreal.ResourceType.WATER,100),(unreal.ResourceType.IRON,100),(unreal.ResourceType.CONCRETE,100),(unreal.ResourceType.CONTROL_CHIP,10),(unreal.ResourceType.ENERGY,1000),(unreal.ResourceType.POLYMER,0)]:
 manager.set_resource_amount(res,amount)
from editor_toolset.toolsets.object import ObjectTools
configs=json.loads(ObjectTools.get_properties(toolbar,["Buttons"]))["Buttons"]
assert any(c["tool"]=="PolymerPlant" and c["iconTexture"] for c in configs)
toolbar.call_method("HandleIndustryCategoryClicked",())
controller.set_active_build_tool(unreal.STPBuildTool.POLYMER_PLANT)
assert controller.get_active_build_tool()==unreal.STPBuildTool.POLYMER_PLANT
surface=actors(unreal.PlanetSurfaceManager)[0]
footprint=unreal.get_default_object(cls).get_grid_footprint()
placement=None
for distance in [850,1100,1400,1800,2200]:
 for direction in [unreal.Vector(0,1,0),unreal.Vector(1,0,0),unreal.Vector(0,-1,0),unreal.Vector(-1,0,0)]:
  candidate=surface.get_building_placement_for_world_location(camp.get_actor_location()+direction*distance,footprint)
  if candidate.valid:
   placement=candidate
   break
 if placement:break
assert placement,"No valid placement near the camp"
location=placement.world_location
plant=spawn(unreal.Transform(location=location,rotation=placement.world_rotation))
assert surface.reserve_cells(plant,placement.origin_cell,plant.get_grid_footprint())
assert manager.try_spend_costs(list(definition.get_editor_property("construction_costs")))
assert isinstance(plant,unreal.PolymerPlant)
comps={c.get_name():c for c in plant.get_components_by_class(unreal.StaticMeshComponent)}
assert comps["BuildingMesh"].static_mesh and comps["FanA"].static_mesh and comps["FanB"].static_mesh and comps["OutputBlock"].static_mesh
# Check visible mesh contact against terrain, not a configured grid offset.
hit=unreal.SystemLibrary.line_trace_single(world,location+unreal.Vector(0,0,500),location-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,actors(unreal.BaseBuilding),unreal.DrawDebugTrace.NONE,True)
hit_fields=statics.call_method("BreakHitResult",(hit,))
assert hit_fields[0],"No terrain collision below factory"
ground=hit_fields[5]
mesh_ground=comps["BuildingMesh"].get_world_location().z+comps["BuildingMesh"].static_mesh.get_bounding_box().min.z
assert abs(mesh_ground-ground.z)<1.0,f"Factory floats above ground: mesh={mesh_ground}, terrain={ground.z}"
assert comps["FanA"].get_attach_parent()==comps["BuildingMesh"]
assert comps["OutputBlock"].get_attach_parent()==comps["BuildingMesh"]
light=plant.get_components_by_class(unreal.PointLightComponent)[0]
preview=None
assert plant.get_construction_progress()<1
unreal.GameplayStatics.set_game_paused(world,False)
unreal.GameplayStatics.set_global_time_dilation(world,10)
started=time.monotonic();phase=0;report={"passed":False,"checks":["catalog","Industry UMG button","controller selects placement","Blueprint meshes","terrain contact within 1 cm and attached animation","valid grid placement and reserved footprint","construction cost payment","preview gate","polymer HUD bindings"],"footprint":str(plant.get_grid_footprint())}
def rotation():return str(comps["FanA"].get_editor_property("relative_rotation"))
def finish():
 (root/"Saved/PolymerRuntimeTest.json").write_text(json.dumps(report,indent=2))
 unreal.unregister_slate_post_tick_callback(handle)
 unreal.GameplayStatics.set_global_time_dilation(world,1)
 if preview:preview.destroy_actor()
 controller.set_active_build_tool(unreal.STPBuildTool.NONE)
 unreal.log("POLYMER_TEST_RESULT "+json.dumps(report))
def check(delta):
 global phase,started,before_rotation,produced,starved_amount,starved_rotation,preview
 if time.monotonic()-started < (2.4 if phase==1 else .35):return
 try:
  if phase==0:
   previews=[a for a in actors(unreal.PolymerPlant) if a.is_placement_preview()]
   assert previews,"Controller did not create polymer placement preview"
   assert not previews[0].is_operational()
   controller.set_active_build_tool(unreal.STPBuildTool.NONE)
   assert not plant.is_producing(),"Incomplete building produced"
   assert light.get_editor_property("intensity")==0
   plant.set_construction_progress(1);grid.refresh_energy_grid()
   assert plant.is_connected_to_power_grid(),"Placement outside base coverage"
   assert plant.is_operational(),"Real energy grid did not power plant"
   before_rotation=rotation();phase=1;started=time.monotonic();return
  if phase==1:
   produced=manager.get_resource_amount(unreal.ResourceType.POLYMER)
   assert produced>=3,"No polymer produced"
   assert produced%3==0
   cycles=produced//3
   assert manager.get_resource_amount(unreal.ResourceType.COAL)==100-2*cycles,"Coal recipe imbalance"
   assert manager.get_resource_amount(unreal.ResourceType.WATER)==100-cycles,"Water recipe imbalance"
   assert rotation()!=before_rotation,"Fans did not rotate"
   assert light.get_editor_property("intensity")>0,"No active light VFX"
   assert comps["OutputBlock"].is_visible()
   assert str(display.get_editor_property("polymer_amount_text").get_text())==str(produced)
   report.update({"produced":produced,"real_grid_power_verified":True})
   report["checks"]+=["completed building production","coal/water conservation","fan rotation","pulsing light","output block visibility","live HUD amount"]
   manager.set_resource_amount(unreal.ResourceType.COAL,0)
   starved_amount=produced;phase=2;started=time.monotonic();return
  if phase==2:
   assert not plant.is_producing(),"Coal starvation did not stop factory"
   assert manager.get_resource_amount(unreal.ResourceType.POLYMER)==starved_amount
   assert light.get_editor_property("intensity")==0
   assert not comps["OutputBlock"].is_visible()
   starved_rotation=rotation();phase=3;started=time.monotonic();return
  if phase==3:
   assert rotation()==starved_rotation,"Fans kept running when starved"
   manager.set_resource_amount(unreal.ResourceType.COAL,100);manager.set_resource_amount(unreal.ResourceType.WATER,0)
   phase=4;started=time.monotonic();return
  if phase==4:
   assert not plant.is_producing(),"Water starvation did not stop factory"
   manager.set_resource_amount(unreal.ResourceType.WATER,100)
   plant.set_actor_location(camp.get_actor_location()+unreal.Vector(30000,0,0),False,False)
   grid.refresh_energy_grid();assert not plant.is_operational()
   phase=5;started=time.monotonic();return
  if phase==5:
   assert not plant.is_producing() and light.get_editor_property("intensity")==0,"No-power VFX gate failed"
   plant.set_actor_location(location,False,False);grid.refresh_energy_grid()
   phase=6;started=time.monotonic();return
  assert plant.is_producing(),"Factory failed to resume"
  assert light.get_editor_property("intensity")>0
  report["checks"]+=["coal starvation stops output and animation","water starvation stops","outside coverage stops","restart resumes"]
  report["passed"]=True
  finish()
 except Exception as error:
  report["error"]=repr(error);finish();unreal.log_error("POLYMER_TEST_FAILED "+repr(error))
handle=unreal.register_slate_post_tick_callback(check)





