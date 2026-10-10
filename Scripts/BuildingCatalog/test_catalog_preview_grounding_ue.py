import unreal,json,time
from pathlib import Path
ground_out=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
ground_world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
ground_controller=unreal.GameplayStatics.get_all_actors_of_class(ground_world,unreal.SurviveThePlanetPlayerController)[0]
ground_rows=json.loads((ground_out/'ue_gameplay_registration.json').read_text())
ground_results=[]
ground_index=0
ground_started=time.monotonic()
def select_ground_preview():
    data=unreal.load_asset(ground_rows[ground_index]['definition']);tool=data.get_editor_property('build_tool')
    ground_controller.get_editor_property('cheat_manager').grant_building_blueprint(tool)
    ground_controller.set_active_build_tool(tool)
select_ground_preview()
def inspect_ground_preview(delta):
    global ground_index,ground_started
    if time.monotonic()-ground_started<0.15:return
    actor=ground_controller.get_active_placement_preview()
    assert actor
    position=actor.get_actor_location()
    ignored=list(unreal.GameplayStatics.get_all_actors_of_class(ground_world,unreal.BaseBuilding))
    hit=unreal.SystemLibrary.line_trace_single(ground_world,position+unreal.Vector(0,0,500),position-unreal.Vector(0,0,1500),unreal.TraceTypeQuery.ECC_VISIBILITY,True,ignored,unreal.DrawDebugTrace.NONE)
    assert hit
    point=unreal.get_default_object(unreal.GameplayStatics).call_method('BreakHitResult',(hit,))[5]
    body=next(c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
    error=body.get_world_location().z+body.static_mesh.get_bounding_box().min.z*body.get_world_scale().z-point.z
    ground_results.append({'key':ground_rows[ground_index]['key'],'bottom_minus_ground_cm':error})
    ground_index+=1
    if ground_index==len(ground_rows):
        unreal.unregister_slate_post_tick_callback(ground_handle)
        ground_controller.set_active_build_tool(unreal.STPBuildTool.NONE)
        report={'passed':all(abs(r['bottom_minus_ground_cm'])<=1 for r in ground_results),'buildings':len(ground_results),'results':ground_results}
        (ground_out/'ue_catalog_preview_grounding_test.json').write_text(json.dumps(report,indent=2));unreal.log('CATALOG_PREVIEW_GROUNDING '+str(report['passed']))
        return
    select_ground_preview();ground_started=time.monotonic()
ground_handle=unreal.register_slate_post_tick_callback(inspect_ground_preview)
