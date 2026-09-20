import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/PlanetLevel')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

resources = [actor for actor in actors.get_all_level_actors()
             if isinstance(actor, unreal.BaseResourceSource)]
for actor in resources:
    location = actor.get_actor_location()
    owner = actor.get_owner()
    unreal.log('AUTHORED_RESOURCE label={} class={} location=({:.1f},{:.1f},{:.1f}) owner={}'.format(
        actor.get_actor_label(), actor.get_class().get_name(),
        location.x, location.y, location.z,
        owner.get_actor_label() if owner else 'None'))
unreal.log('AUTHORED_RESOURCE_COUNT {}'.format(len(resources)))
