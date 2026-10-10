"""Verify all catalog foundations against terrain in PIE, including moved previews."""
import unreal,json
from pathlib import Path
def test_grounding():
    out=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
    rows=json.loads((out/'ue_gameplay_registration.json').read_text())
    statics=unreal.get_default_object(unreal.GameplayStatics)
    ignored=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding))
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    if pawn:pawn.set_actor_tick_enabled(False)
    results=[]
    for row in rows:
        cls=unreal.load_class(None,row['class']);assert unreal.get_default_object(cls).get_editor_property('ground_mesh_to_surface')
        errors=[]
        for preview,x,y in [(False,1480,-370),(False,1800,-500)]:
            hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x,y,1000),unreal.Vector(x,y,-1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignored,unreal.DrawDebugTrace.NONE)
            assert hit,row['key']
            parts=statics.call_method('BreakHitResult',(hit,))
            ground=parts[5]
            transform=unreal.Transform(location=unreal.Vector(x,y,ground.z+186))
            actor=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            if preview:
                actor.set_placement_preview(True)
                actor.set_actor_location(unreal.Vector(x+20,y,ground.z+186),False,False)
                actor.set_placement_preview_valid(True)
                hit=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(x+20,y,1000),unreal.Vector(x+20,y,-1500),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignored+[actor],unreal.DrawDebugTrace.NONE)
                ground=statics.call_method('BreakHitResult',(hit,))[5]
            body=next(c for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
            bounds=body.static_mesh.get_bounding_box()
            bottom=body.get_world_location().z+bounds.min.z*body.get_world_scale().z
            error=bottom-ground.z
            errors.append({'preview':preview,'bottom_minus_ground_cm':error,'root_z':actor.get_actor_location().z,'body_offset_z':body.get_editor_property('relative_location').z})
            assert abs(error)<=1.0,(row['key'],error)
            assert all(c.get_attach_parent()==body for c in actor.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='Motion')
            actor.destroy_actor()
        results.append({'key':row['key'],'checks':errors})
    report={'passed':True,'buildings':len(results),'checks':sum(len(r['checks']) for r in results),'max_absolute_error_cm':max(abs(c['bottom_minus_ground_cm']) for r in results for c in r['checks']),'results':results}
    (out/'ue_catalog_grounding_test.json').write_text(json.dumps(report,indent=2));unreal.log('CATALOG_GROUNDING_PASS_49')
test_grounding()
