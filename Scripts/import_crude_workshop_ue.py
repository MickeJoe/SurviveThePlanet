import unreal, json, traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path('C:/UE5/SurviveThePlanet 5.8')
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
src=root/'ContentSource/CrudeWorkshop';dest='/Game/Units/Buildings/CrudeWorkshop'
def run():
 meshes={}
 for name in ['SM_CrudeWorkshop','SM_WorkshopFan','SM_WorkshopRam','T_CrudeWorkshop']:
  texture=name.startswith('T_');task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_path=dest;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
  if not texture:
   opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False
   opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;opt.static_mesh_import_data.generate_lightmap_u_vs=True;task.options=opt
  tools.import_asset_tasks([task]);asset=unreal.load_asset(dest+'/'+name);assert asset,name
  if not texture:
   for i,slot in enumerate(asset.get_editor_property('static_materials')):
    key=next(k for k in ['Ivory','Orange','Graphite','Steel','Cyan'] if k in str(slot.material_slot_name))
    material_path='/Game/Units/Buildings/EnergyExtender/Extender_Cyan' if key=='Cyan' else '/Game/Geothermal/M_GeoDetail_'+('Dark' if key=='Graphite' else key)
    mat=unreal.load_asset(material_path);assert mat,key
    asset.set_material(i,mat)
   meshes[name]=asset
  else:
   icon=asset;icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);icon.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
  assert lib.save_loaded_asset(asset)
 data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop');assert data
 # Preserve ownership identity and the old catalog Blueprint for existing saves; use a new functional class.
 bp=unreal.load_asset(dest+'/BP_CrudeWorkshop')
 if not bp:
  factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.CrudeWorkshop)
  bp=tools.create_asset('BP_CrudeWorkshop',dest,unreal.Blueprint,factory)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);cdo=unreal.get_default_object(bp.generated_class())
 cost=unreal.ResourceCost(resource=unreal.ResourceType.IRON,cost=10)
 for key,value in {'display_name':'Crude Workshop','description':'Drone-operated starter workshop. Choose concrete, steel or copper. Requires assigned drones and 6 electricity/minute.','building_mesh':meshes['SM_CrudeWorkshop'],'thumbnail':icon,'toolbar_icon':icon,'blueprint_initially_owned':True,'show_in_build_toolbar':True,'build_category':unreal.STPBuildCategory.INDUSTRY,'toolbar_sort_order':5,'max_drone_slots':4,'initially_unlocked_drone_slots':4,'drone_work_type':unreal.STPDroneWorkType.CONSTRUCTION,'override_energy_settings':True,'energy_consumption_per_minute':6.0,'energy_production_per_minute':0.0,'energy_storage_capacity':0.0,'construction_costs':[cost],'building_class':bp.generated_class()}.items():data.set_editor_property(key,value)
 for key,value in {'building_data':data,'fan_mesh':meshes['SM_WorkshopFan'],'ram_mesh':meshes['SM_WorkshopRam'],'construction_progress':0.0}.items():cdo.set_editor_property(key,value)
 body=next(c for c in cdo.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
 body.set_static_mesh(meshes['SM_CrudeWorkshop']);body.set_editor_property('relative_rotation',unreal.Rotator(pitch=0,yaw=180,roll=0))
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 for asset in [bp,data]:assert lib.save_loaded_asset(asset)
 assert data in list(unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog').get_editor_property('buildings'))
 # Verify that Blueprint defaults persist across reload, including imported mesh and native movable parts.
 unreal.EditorLoadingAndSavingUtils.reload_packages([bp.get_outer()]);bp=unreal.load_asset(dest+'/BP_CrudeWorkshop');cdo=unreal.get_default_object(bp.generated_class())
 assert cdo.get_editor_property('fan_mesh') and cdo.get_editor_property('ram_mesh')
 return {'passed':True,'class':bp.generated_class().get_path_name(),'footprint':str(cdo.get_grid_footprint()),'meshes':{k:str(v.get_bounding_box()) for k,v in meshes.items()},'shared_materials':True,'starting_blueprint':True,'cost':'10 iron ore','native_actor':True}
try:report=run()
except Exception:report={'passed':False,'error':traceback.format_exc()}
(root/'Saved/CrudeWorkshopImport.json').write_text(json.dumps(report,indent=2));unreal.log('CRUDE_WORKSHOP_IMPORT '+json.dumps(report))
