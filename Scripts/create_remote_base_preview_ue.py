import unreal, json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Preserve unsaved maps before opening preview'
previous=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name()
dest='/Game/Units/Buildings/RemoteBase'
preview=dest+'/Preview/L_RemoteBasePreview'
assert not unreal.EditorAssetLibrary.does_asset_exist(preview), 'Preview already exists; do not overwrite'
assert levels.new_level(preview)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase)

def spawn(cls,name,location,rotation=unreal.Rotator()):
    actor=actors.spawn_actor_from_class(cls,unreal.Vector(*location),rotation)
    assert actor,name
    actor.set_actor_label(name)
    return actor

bp=unreal.load_asset(dest+'/BP_RemoteBase')
remote=spawn(bp.generated_class(),'Remote Base - Animated',(430,0,2))
old=spawn(unreal.StaticMeshActor,'Existing Main Base - Scale Reference',(-500,0,280))
old.static_mesh_component.set_static_mesh(unreal.load_asset('/Game/Models/Buildings/BaseModuleUsed/BaseModule'))
old.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
ground=spawn(unreal.StaticMeshActor,'Preview Ground',(0,0,-12))
ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
ground.set_actor_scale3d(unreal.Vector(100,100,.24))
tools=unreal.AssetToolsHelpers.get_asset_tools()
mat=tools.create_asset('M_RemoteBase_PreviewSand',dest+'/Preview',unreal.Material,unreal.MaterialFactoryNew())
mel=unreal.MaterialEditingLibrary
n=mel.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,0)
n.constant=unreal.LinearColor(.18,.13,.07,1)
mel.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
n=mel.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,160);n.r=.9
mel.connect_material_property(n,'',unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
ground.static_mesh_component.set_material(0,mat)
sun=spawn(unreal.DirectionalLight,'Preview Sun',(0,0,1000),unreal.Rotator(pitch=-48,yaw=-35,roll=0))
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property('intensity',3.0)
sun.light_component.set_editor_property('light_color',unreal.Color(255,239,214,255))
sky=spawn(unreal.SkyLight,'Preview Sky',(0,0,800))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('intensity',1.0)
spawn(unreal.SkyAtmosphere,'Preview Atmosphere',(0,0,-100))
sky.light_component.set_editor_property('real_time_capture',True)
post=spawn(unreal.PostProcessVolume,'Preview Fixed Exposure',(0,0,0))
post.set_editor_property('unbound',True)
settings=post.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_min_brightness',True)
settings.set_editor_property('override_auto_exposure_max_brightness',True)
settings.set_editor_property('auto_exposure_min_brightness',1.0)
settings.set_editor_property('auto_exposure_max_brightness',1.0)
post.set_editor_property('settings',settings)
camera=spawn(unreal.CameraActor,'Preview Camera',(1650,2350,1900),unreal.Rotator(pitch=-31,yaw=-126,roll=0))
camera.camera_component.set_editor_property('field_of_view',48.0)
camera.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
levels.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation(),unreal.Name('None'))
levels.save_current_level()
report={'preview_map':preview,'previous_level':previous,'blueprint':bp.get_path_name(),'actor':remote.get_path_name(),'components':[(c.get_name(),str(c.get_relative_transform())) for c in remote.get_components_by_class(unreal.SceneComponent)]}
(root/'Saved'/'RemoteBase_preview.json').write_text(json.dumps(report,indent=2))
unreal.log('REMOTE_BASE_PREVIEW_READY '+json.dumps(report))
