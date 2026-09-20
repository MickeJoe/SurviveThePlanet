import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for map_path in ['/Game/PlanetLevel_RockPreview', '/Game/PlanetLevel']:
    assert levels.load_level(map_path)
    unreal.log('RENDER_MAP {}'.format(map_path))
    for actor in actors.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if class_name in ('DirectionalLight', 'SkyLight', 'PostProcessVolume'):
            unreal.log('RENDER_ACTOR class={} label={}'.format(class_name, actor.get_actor_label()))
            for component in actor.get_components_by_class(unreal.ActorComponent):
                for prop in ('intensity', 'light_color', 'indirect_lighting_intensity',
                             'source_angle', 'mobility', 'settings'):
                    try:
                        unreal.log('RENDER_PROP {}.{}={}'.format(component.get_class().get_name(), prop,
                                                                 component.get_editor_property(prop)))
                    except Exception:
                        pass

    for actor in actors.get_all_level_actors():
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property('static_mesh')
            if not mesh or not any(name in mesh.get_name() for name in ('Alien_Rosette', 'Alien_Coral', 'Cliff_Wall')):
                continue
            overrides = component.get_editor_property('override_materials')
            unreal.log('RENDER_MESH mesh={} overrides={}'.format(mesh.get_path_name(),
                [m.get_path_name() if m else 'None' for m in overrides]))
            break
