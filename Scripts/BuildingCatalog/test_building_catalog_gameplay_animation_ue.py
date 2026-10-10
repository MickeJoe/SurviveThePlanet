"""Check mechanical components and process effects on all playable catalog classes in PIE."""
import unreal,json,time
from pathlib import Path
OUT=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
records=json.loads((OUT/'BuildingCatalog/authored_manifest.json').read_text())
statics=unreal.get_default_object(unreal.GameplayStatics)
samples=[]
for index,r in enumerate(records):
    cls=unreal.load_class(None,'/Game/Units/Buildings/BlueprintCatalog/'+r['key']+'/BP_'+r['key']+'Building.BP_'+r['key']+'Building_C')
    transform=unreal.Transform(location=unreal.Vector(10000+(index%7)*1200,10000+(index//7)*1200,200))
    actor=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    comps=actor.get_components_by_class(unreal.StaticMeshComponent)
    motion=next(c for c in comps if c.get_name()=='Motion')
    assert len(comps)==2 and motion.get_attach_parent().get_name()=='BuildingMesh',r['key']
    samples.append((r,actor,motion,motion.get_relative_transform()))
unreal.GameplayStatics.set_game_paused(world,False)
started=time.monotonic()
def check_catalog_gameplay_animation(delta):
    if time.monotonic()-started<2:return
    unreal.unregister_slate_post_tick_callback(catalog_gameplay_animation_handle)
    results=[]
    for r,actor,motion,before in samples:
        results.append({'key':r['key'],'moved':before!=motion.get_relative_transform(),
            'fx_active':all(c.is_active() for c in actor.get_components_by_class(unreal.NiagaraComponent)),
            'motion_follows_body':motion.get_attach_parent().get_name()=='BuildingMesh'})
        actor.destroy_actor()
    report={'passed':all(r['moved'] and r['fx_active'] and r['motion_follows_body'] for r in results),'tested':len(results),'results':results}
    (OUT/'ue_gameplay_animation_runtime.json').write_text(json.dumps(report,indent=2))
    unreal.log('CATALOG_GAMEPLAY_ANIMATION_TEST '+str(report['passed']))
catalog_gameplay_animation_handle=unreal.register_slate_post_tick_callback(check_catalog_gameplay_animation)
