"""Replace uranium mine art while preserving its resource, drone and balance data."""
import unreal,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path('C:/UE5/SurviveThePlanet 5.8');src=root/'ContentSource/UraniumMine';dest='/Game/Uranium'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
from editor_toolset.toolsets.actor import ActorTools
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def import_asset(name,texture=False):
 task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_path=dest;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt
 tools.import_asset_tasks([task]);asset=unreal.load_asset(dest+'/'+name);assert asset;return asset
try:
 data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_UraniumMine');bp=unreal.load_asset(dest+'/BP_UraniumMine');assert data and bp
 cdo=unreal.get_default_object(bp.generated_class())
 properties=['construction_costs','max_drone_slots','initially_unlocked_drone_slots','energy_consumption_per_minute','energy_production_per_minute','build_tool']
 before={p:str(data.get_editor_property(p)) for p in properties}
 meshes={n:import_asset(n) for n in ['SM_UraniumMine_Refined','SM_UraniumMine_Drill']}
 shared_materials=set()
 for mesh in meshes.values():
  for i,slot in enumerate(mesh.static_materials):
   key=next(k for k in ['Ivory','Orange','Dark','Steel','Cyan','Lamp'] if 'Geo_'+k in str(slot.material_slot_name))
   path='/Game/Uranium/M_NuclearStatus' if key=='Cyan' else '/Game/Geothermal/M_GeoDetail_'+key
   mat=unreal.load_asset(path);assert mat;mesh.set_material(i,mat);shared_materials.add(path)
  lib.save_loaded_asset(mesh)
 body=next(c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='BuildingMesh')
 body.set_static_mesh(meshes['SM_UraniumMine_Refined']);body.set_editor_property('override_materials',[]);body.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
 drill=next((c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name().startswith('MineDrill')),None) or ActorTools.add_component(bp,unreal.StaticMeshComponent.static_class(),'MineDrill')
 drill.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
 body=next(c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='BuildingMesh')
 assert ActorTools.set_parent_component(drill,body)
 drill.set_static_mesh(meshes['SM_UraniumMine_Drill']);drill.set_editor_property('relative_location',unreal.Vector(-80,0,44));drill.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
 effect=next((c for c in components(bp) if isinstance(c,unreal.IndustrialEffectsComponent)),None) or ActorTools.add_component(bp,unreal.IndustrialEffectsComponent.static_class(),'IndustrialEffects')
 effect.set_editor_property('effect_kind',unreal.IndustrialEffectKind.URANIUM_MINE)
 cdo.set_editor_property('ground_mesh_to_surface',True);data.set_editor_property('building_mesh',meshes['SM_UraniumMine_Refined'])
 icon=import_asset('T_UraniumMine_Refined',True);icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
 data.set_editor_property('toolbar_icon',icon);data.set_editor_property('thumbnail',icon)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
 data.set_editor_property('building_class',bp.generated_class());lib.save_loaded_asset(bp);lib.save_loaded_asset(data)
 assert before=={p:str(data.get_editor_property(p)) for p in properties},'Existing costs/power/drone data must remain unchanged'
 source=unreal.get_default_object(unreal.load_asset(dest+'/BP_UraniumResource').generated_class());assert source.get_mine_blueprint()==bp.generated_class()
 result={'passed':True,'balance_and_drone_data_preserved':before,'shared_materials':sorted(shared_materials),'bounds':str(meshes['SM_UraniumMine_Refined'].get_bounding_box()),'blueprint':bp.get_path_name()}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/UraniumMineImport.json').write_text(json.dumps(result,indent=2));unreal.log('URANIUM_MINE_IMPORT '+json.dumps(result))
