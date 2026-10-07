import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
buildings=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding))
factory=next(a for a in buildings if a.actor_has_tag("ConnectorFactory"))
assert factory.get_building_type()==unreal.STPBuildingType.OTHER
assert factory.get_construction_progress()==1.0
mesh=next(c for c in factory.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=="BuildingMesh")
loc=factory.get_actor_location();hit=unreal.SystemLibrary.line_trace_single(world,loc+unreal.Vector(0,0,500),loc-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,buildings,unreal.DrawDebugTrace.NONE,True)
fields=unreal.get_default_object(unreal.GameplayStatics).call_method("BreakHitResult",(hit,));assert fields[0],"No ground";ground=fields[5]
bottom=mesh.get_world_location().z+mesh.static_mesh.get_bounding_box().min.z
surface=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetSurfaceManager)[0]
origin=surface.try_get_actor_origin_cell(factory)
report={"passed":abs(bottom-ground.z)<1,"actor":factory.get_path_name(),"ground_z":ground.z,"mesh_bottom_z":bottom,"ground_error_cm":bottom-ground.z,"grid_reservation":str(origin),"footprint":str(factory.get_grid_footprint()),"position":[loc.x,loc.y,loc.z],"construction_complete":True,"factory_has_base_tag":factory.actor_has_tag("BaseModule")}
(root/"Saved/ConnectorRuntimeTest.json").write_text(json.dumps(report,indent=2));unreal.log("CONNECTOR_RUNTIME "+json.dumps(report))
unreal.GameplayStatics.set_game_paused(world,True)
