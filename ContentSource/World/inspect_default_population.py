import unreal
editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert editor.load_level('/Game/PlanetLevel')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
population = next(a for a in actors.get_all_level_actors() if isinstance(a, unreal.SectorPopulation))
population.initialize_population()
unreal.log('POPULATION_DIAGNOSTICS ' + str(population.get_editor_property('diagnostics')))
unreal.log('POPULATION_DECORATIONS ' + str(len(population.get_editor_property('decorations'))))
unreal.log('POPULATION_DEPOSITS ' + str(len(population.get_editor_property('resources').get_editor_property('deposits'))))
# No save: inspection-created actors are discarded when this commandlet exits.
from collections import Counter
decorations = population.get_editor_property('decorations')
counts = Counter(d.get_editor_property('sector_id') for d in decorations)
assert len(counts) == 37 and all(n >= 48 for n in counts.values()), counts
assert not population.get_editor_property('diagnostics')

def spawned_counts():
    deposits = [a for a in actors.get_all_level_actors()
                if isinstance(a, unreal.BaseResourceSource) and a.get_owner() == population]
    instances = sum(c.get_instance_count() for c in population.get_components_by_class(unreal.InstancedStaticMeshComponent))
    return len(deposits), instances

expected_start_instances = counts[0]
assert spawned_counts() == (1, expected_start_instances), spawned_counts()
grid = population.get_editor_property('grid')
for sector_id in sorted(counts):
    grid.set_sector_state(sector_id, unreal.SectorState.DISCOVERED)
expected_all_instances = sum(counts.values())
assert spawned_counts() == (37, expected_all_instances), spawned_counts()
for sector_id in sorted(counts):
    population.reveal_sector(sector_id)
population.initialize_population()
assert spawned_counts() == (37, expected_all_instances), spawned_counts()
unreal.log('POPULATION_DISCOVERY_VERIFIED: initial 1/{}; all 37/{}; repeated reveal unchanged'.format(
    expected_start_instances, expected_all_instances))
