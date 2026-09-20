import unreal
from collections import defaultdict

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/PlanetLevel_RockPreview')

rows = defaultdict(list)
for actor in actors.get_all_level_actors():
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = component.get_editor_property('static_mesh')
        if not mesh or '/Environment/' not in mesh.get_path_name():
            continue
        actor_scale = actor.get_actor_scale3d()
        component_scale = component.get_editor_property('relative_scale3d')
        scale = unreal.Vector(actor_scale.x * component_scale.x,
                              actor_scale.y * component_scale.y,
                              actor_scale.z * component_scale.z)
        materials = tuple(m.get_path_name() if m else 'None' for m in component.get_materials())
        rows[mesh.get_path_name()].append((scale.x, scale.y, scale.z, materials))

for mesh_name, instances in sorted(rows.items()):
    scales = [max(abs(v[0]), abs(v[1]), abs(v[2])) for v in instances]
    material_sets = sorted(set(v[3] for v in instances))
    unreal.log('COMPOSITION mesh={} count={} scale_min={:.3f} scale_avg={:.3f} scale_max={:.3f}'.format(
        mesh_name, len(instances), min(scales), sum(scales) / len(scales), max(scales)))
    for materials in material_sets:
        unreal.log('COMPOSITION_MATERIAL mesh={} materials={}'.format(mesh_name, materials))
