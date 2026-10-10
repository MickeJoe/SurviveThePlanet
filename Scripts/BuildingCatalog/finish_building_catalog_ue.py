"""Attach Niagara effects and create a separate review level, preserving gameplay maps."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.actor import ActorTools
ROOT=Path(unreal.Paths.project_dir()).resolve();assert ROOT==Path(r'C:\UE5\SurviveThePlanet 5.8')
OUT=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
DEST='/Game/Units/Buildings/BlueprintCatalog';lib=unreal.EditorAssetLibrary
records=json.loads((OUT/'BuildingCatalog/authored_manifest.json').read_text())
vapor=unreal.load_asset(DEST+'/FX/NS_CatalogProcessVapor');assert vapor;assert lib.save_loaded_asset(vapor)
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
for r in records:
 if r['id'] not in (28,30,33,34,35,44,51,54,55,69):continue
 bp=unreal.load_asset(DEST+'/'+r['key']+'/BP_'+r['key'])
 fx=next((o for o in components(bp) if isinstance(o,unreal.NiagaraComponent)),None)
 if not fx:fx=ActorTools.add_component(bp,unreal.NiagaraComponent.static_class(),'ProcessVapor')
 fx.set_asset(vapor);fx.set_editor_property('auto_activate',True)
 fx.set_editor_property('relative_location',unreal.Vector(-165,-80,300+25*(r['id']%3)))
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert lib.save_loaded_asset(bp)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path=DEST+'/Preview/L_BuildingCatalogReview'
dirty=unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
if dirty:
 assert all(p.get_path_name().startswith(DEST+'/Preview/') for p in dirty),'Preserve unrelated unsaved maps'
 assert levels.save_current_level()
current_map=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]
if lib.does_asset_exist(map_path):
 if current_map!=map_path:assert levels.load_level(map_path)
else:assert levels.new_level(map_path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase)
def spawn(cls,label,location,rotation=unreal.Rotator()):
 actor=actors.spawn_actor_from_class(cls,unreal.Vector(*location),rotation);assert actor;actor.set_actor_label(label);return actor
existing_labels={a.get_actor_label() for a in actors.get_all_level_actors()}
asset_report=[]
for k,r in enumerate(records):
 bp=unreal.load_asset(DEST+'/'+r['key']+'/BP_'+r['key']);x=(k%7)*1000;y=(k//7)*1000
 label='%02d %s'%(r['id'],r['name'])
 if label not in existing_labels:
  actor=spawn(bp.generated_class(),label,(x,y,0));actor.set_folder_path(unreal.Name('New Buildings'))
  caption=spawn(unreal.TextRenderActor,'Label '+label,(x-290,y-420,2),unreal.Rotator(pitch=90,yaw=90,roll=0));caption.text_render.set_text(unreal.Text(label));caption.text_render.set_world_size(25)
 comps=unreal.get_default_object(bp.generated_class()).static_mesh_component
 mesh=comps.get_editor_property('static_mesh');extent=mesh.get_bounding_box()
 assert 450<extent.max.x-extent.min.x<1100,(r['key'],extent)
 assert len(mesh.get_editor_property('static_materials'))>=4,r['key']
 asset_report.append({'key':r['key'],'body_bounds_cm':str(extent),'blueprint_status':str(bp.get_editor_property('status'))})
inventory=json.loads((OUT/'building_inventory.json').read_text());existing_report=[]
for r in inventory:
 if not r['existing_asset']:continue
 asset=unreal.load_asset(r['existing_asset']);assert isinstance(asset,unreal.StaticMesh),(r['name'],str(asset))
 existing_report.append({'id':r['id'],'name':r['name'],'asset':asset.get_path_name(),'bounds':str(asset.get_bounding_box())})
if 'Existing Mother Base - Style Reference' not in existing_labels:
 mesh=unreal.load_asset('/Game/Models/Buildings/BaseModuleUsed/BaseModule');b=mesh.get_bounding_box()
 base=spawn(unreal.StaticMeshActor,'Existing Mother Base - Style Reference',(-1400,0,-b.min.z));base.static_mesh_component.set_static_mesh(mesh)
 ground=spawn(unreal.StaticMeshActor,'Review ground',(2800,2800,-12));ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));ground.set_actor_scale3d(unreal.Vector(100,100,.24))
 ground.static_mesh_component.set_material(0,unreal.load_asset('/Game/Units/Buildings/RemoteBase/Preview/M_RemoteBase_PreviewSand'))
 sun=spawn(unreal.DirectionalLight,'Review Sun',(0,0,1000),unreal.Rotator(pitch=-48,yaw=-35,roll=0));sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sun.light_component.set_editor_property('intensity',4.0)
 sky=spawn(unreal.SkyLight,'Review Sky',(0,0,800));sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sky.light_component.set_editor_property('intensity',1.2);sky.light_component.set_editor_property('real_time_capture',True)
 spawn(unreal.SkyAtmosphere,'Review Atmosphere',(0,0,-100))
 post=spawn(unreal.PostProcessVolume,'Review exposure',(0,0,0));post.set_editor_property('unbound',True);s=post.get_editor_property('settings')
 for p,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',1.0),('auto_exposure_max_brightness',1.0)]:s.set_editor_property(p,v)
 post.set_editor_property('settings',s)
levels.set_level_viewport_camera_info(unreal.Vector(2800,-5500,7500),unreal.Rotator(pitch=-42,yaw=88,roll=0),unreal.Name('None'))
assert levels.save_current_level()
(OUT/'ue_validation.json').write_text(json.dumps({'new_buildings':len(asset_report),'existing_buildings':len(existing_report),'meshes':len(asset_report)*2,'preview_map':map_path,'new_assets':asset_report,'existing_assets':existing_report},indent=2))
unreal.log('CATALOG_REVIEW_READY')
