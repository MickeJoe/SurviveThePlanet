import unreal
from collections import Counter

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for map_path in ['/Game/PlanetLevel_RockPreview', '/Game/Environment/Expedition02/PlanetLevel_Before02']:
    if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
        continue
    assert levels.load_level(map_path)
    meshes = Counter()
    materials = Counter()
    for actor in actors.get_all_level_actors():
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property('static_mesh')
            if mesh:
                meshes[mesh.get_path_name()] += max(1, component.get_instance_count()) if isinstance(component, unreal.InstancedStaticMeshComponent) else 1
            for material in component.get_materials():
                if material:
                    materials[material.get_path_name()] += 1
    unreal.log('ENVIRONMENT_MAP {}'.format(map_path))
    for name, count in meshes.most_common():
        unreal.log('ENVIRONMENT_MESH {} {}'.format(count, name))
    for name, count in materials.most_common():
        unreal.log('ENVIRONMENT_MATERIAL {} {}'.format(count, name))
