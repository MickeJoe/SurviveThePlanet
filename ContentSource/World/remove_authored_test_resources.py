import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/PlanetLevel')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

resource_classes = {
    'BP_IronSource_C',
    'BP_CopparSource_C',
    'BP_StoneSource_C',
}
resources = [actor for actor in actors.get_all_level_actors()
             if isinstance(actor, unreal.BaseResourceSource)
             and actor.get_owner() is None
             and actor.get_class().get_name() in resource_classes]

assert len(resources) == 9, 'Expected the nine authored test resources, found {}'.format(len(resources))
for actor in resources:
    unreal.log('REMOVING_AUTHORED_TEST_RESOURCE {} {}'.format(
        actor.get_actor_label(), actor.get_class().get_name()))
assert actors.destroy_actors(resources)
assert levels.save_current_level()
unreal.log('REMOVED_AUTHORED_TEST_RESOURCES {}'.format(len(resources)))
