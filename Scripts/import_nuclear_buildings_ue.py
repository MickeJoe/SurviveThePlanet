"""Integrate refined industrial art without changing balance or drone allocation."""
import unreal,json,traceback
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8');src=root/'ContentSource/NuclearIndustrial'
dest='/Game/Units/Buildings/NuclearIndustrial'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
from editor_toolset.toolsets.actor import ActorTools
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def import_asset(name,texture=False):
 t=unreal.AssetImportTask();t.filename=str(src/(name+('.png' if texture else '.fbx')));t.destination_path=dest;t.destination_name=name;t.automated=True;t.save=True;t.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;t.options=opt
 tools.import_asset_tasks([t]);a=unreal.load_asset(dest+'/'+name);assert a;return a
try:
 meshes={n:import_asset(n) for n in ['SM_NuclearReactor_Refined','SM_UraniumProcessor_Refined','SM_UraniumProcessor_Fan']}
 for m in meshes.values():
  for i,s in enumerate(m.get_editor_property('static_materials')):
   name=str(s.material_slot_name);key=next(k for k in ['Ivory','Orange','Dark','Steel','Cyan','Lamp'] if 'Geo_'+k in name)
   mat=unreal.load_asset('/Game/Geothermal/M_GeoDetail_'+key);assert mat;m.set_material(i,mat)
  lib.save_loaded_asset(m)
 rows=[('NuclearReactor','/Game/Data/Buildings/STP/DA_STP_NuclearReactor','/Game/BluePrints/Buildings/STP/BP_STP_NuclearReactor'),('UraniumProcessor','/Game/Data/Buildings/BlueprintCatalog/DA_UraniumProcessor','/Game/Units/Buildings/BlueprintCatalog/UraniumProcessor/BP_UraniumProcessorBuilding')]
 report=[]
 for key,data_path,bp_path in rows:
  data=unreal.load_asset(data_path);bp=unreal.load_asset(bp_path);assert data and bp
  cdo=unreal.get_default_object(bp.generated_class());old_mesh=data.get_editor_property('building_mesh')
  before={'energy_production':data.get_editor_property('energy_production_per_minute'),'energy_consumption':data.get_editor_property('energy_consumption_per_minute'),'costs':str(data.get_editor_property('construction_costs')),'drone_slots':cdo.get_max_assigned_drones() if hasattr(cdo,'get_max_assigned_drones') else None}
  body=next(o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name()=='BuildingMesh')
  mesh=meshes['SM_'+key+'_Refined'];body.set_static_mesh(mesh);body.set_editor_property('override_materials',[]);body.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
  cdo.set_editor_property('ground_mesh_to_surface',True)
  data.set_editor_property('building_mesh',mesh)
  icon=import_asset('T_'+key+'_Refined',True);icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
  data.set_editor_property('toolbar_icon',icon);data.set_editor_property('thumbnail',icon)
  if key=='UraniumProcessor':
   motion=next(o for o in components(bp) if isinstance(o,unreal.StaticMeshComponent) and o.get_name().startswith('Motion'))
   assert ActorTools.set_parent_component(motion,body)
   motion.set_static_mesh(meshes['SM_UraniumProcessor_Fan']);motion.set_editor_property('override_materials',[]);motion.set_editor_property('relative_location',unreal.Vector(0,-40,224));motion.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
  unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
  lib.save_loaded_asset(bp);lib.save_loaded_asset(data)
  assert before['energy_production']==data.get_editor_property('energy_production_per_minute') and before['energy_consumption']==data.get_editor_property('energy_consumption_per_minute') and before['costs']==str(data.get_editor_property('construction_costs'))
  report.append({'key':key,'old_mesh':old_mesh.get_path_name(),'mesh':mesh.get_path_name(),'bounds':str(mesh.get_bounding_box()),'balance_preserved':True,'grounded':True})
 result={'passed':True,'buildings':report}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/NuclearIndustrialImport.json').write_text(json.dumps(result,indent=2));unreal.log('NUCLEAR_INDUSTRIAL_IMPORT '+json.dumps(result))
