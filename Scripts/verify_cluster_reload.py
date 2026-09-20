"""Check that saved preview generation survives reopening the authoring level."""
import unreal
import json
from pathlib import Path

levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First'
levels.load_level('/Game/WorldGeneration/ClusterVariants/Shape03/L_ClusterVariants_Shape03')
levels.load_level(path)
sector=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
generated=sector.get_editor_property('generated_preview')
count=sum(c.get_instance_count() for c in generated.get_components_by_class(unreal.InstancedStaticMeshComponent)) if generated else 0
report={'instances_after_reload':count}
for actor in actors.get_all_level_actors():
    if actor.get_actor_label()=='Authoring_Key':
        actor.light_component.set_editor_property('forward_shading_priority',1)
sector.generate_preview()
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(800,-4800,6500),unreal.Rotator(pitch=-53,yaw=98,roll=0))
levels.save_current_level()
(Path(unreal.Paths.project_dir())/'Saved'/'ClusterReload.json').write_text(json.dumps(report))
unreal.log('CLUSTER_RELOAD '+json.dumps(report))
