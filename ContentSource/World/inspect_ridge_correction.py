import unreal
from collections import Counter
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/PlanetLevel')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
population = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.SectorPopulation))
assert any(a.get_actor_label() == 'E02_SoftFill' for a in actors.get_all_level_actors())
population.initialize_population()
meshes = population.get_editor_property('decoration_meshes')
sets = population.get_editor_property('decoration_material_sets')
decorations = population.get_editor_property('decorations')
counts = Counter(d.get_editor_property('sector_id') for d in decorations)
assert len(counts) == 37
for group in population.get_components_by_class(unreal.InstancedStaticMeshComponent):
    mesh = group.get_editor_property('static_mesh')
    index = list(meshes).index(mesh)
    for slot, material in enumerate(sets[index].get_editor_property('materials')):
        assert material and group.get_material(slot) == material
unreal.log('RIDGE_CORRECTION counts={} coverage={} diagnostics={}'.format(
    dict(counts), population.get_editor_property('actual_terrain_coverage'), population.get_editor_property('diagnostics')))
unreal.log('RIDGE_MATERIALS_AND_FILL_VERIFIED')
# Never save temporary runtime actors.
