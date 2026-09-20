"""Open and light the POC. Capture via the native MCP CaptureViewport tool."""
import unreal
from pathlib import Path

project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
author_variants=globals().get('AUTHOR_VARIANTS',False)
levels.load_level('/Game/WorldGeneration/ClusterVariants/Shape03/L_ClusterVariants_Shape03' if author_variants else '/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First')
if not author_variants:
    sector=next(a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlanetSectorTemplateActor))
    sector.generate_preview()

def existing(label):
    return next((a for a in actors.get_all_level_actors() if a.get_actor_label()==label),None)

if not existing('Authoring_Ground'):
    ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-12))
    ground.set_actor_label('Authoring_Ground')
    ground.set_editor_property('is_editor_only_actor',True)
    ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
    ground.static_mesh_component.set_material(0,unreal.load_asset('/Game/Environment/Expedition02/Materials/M_Terrain_Expedition02'))
    ground.set_actor_scale3d(unreal.Vector(140 if author_variants else 90,90,1))
for name,rotation,intensity in [('Authoring_Key',unreal.Rotator(pitch=-48,yaw=-35,roll=0),5.0),('Authoring_Fill',unreal.Rotator(pitch=-65,yaw=145,roll=0),1.5)]:
    light=existing(name)
    if not light:
        light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),rotation)
        light.set_actor_label(name)
        light.set_editor_property('is_editor_only_actor',True)
        light.light_component.set_editor_property('intensity',intensity)
    light.light_component.set_editor_property('forward_shading_priority',1 if name=='Authoring_Key' else 0)
location=unreal.Vector(0,-4000,6500) if author_variants else unreal.Vector(900,-6000,8500)
rotation=unreal.Rotator(pitch=-58,yaw=90,roll=0) if author_variants else unreal.Rotator(pitch=-53,yaw=98,roll=0)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(location,rotation)
levels.save_current_level()
