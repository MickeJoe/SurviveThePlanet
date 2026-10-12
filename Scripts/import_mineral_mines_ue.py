import unreal,json,traceback
from pathlib import Path
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path('C:/UE5/SurviveThePlanet 5.8')
src=root/'ContentSource/MineralMines';dest='/Game/Resources/MineralMines'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools();sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def import_asset(name,texture=False):
 task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_name=name;task.destination_path=dest;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt
 tools.import_asset_tasks([task]);asset=unreal.load_asset(dest+'/'+name);assert asset
 if not texture:
  for i,slot in enumerate(asset.static_materials):
   key=next(k for k in ['Ivory','Orange','Dark','Steel','Cyan','Lamp'] if 'Geo_'+k in str(slot.material_slot_name))
   path='/Game/Uranium/M_NuclearStatus' if key=='Cyan' else '/Game/Geothermal/M_GeoDetail_'+key
   mat=unreal.load_asset(path);assert mat;asset.set_material(i,mat)
  lib.save_loaded_asset(asset)
 return asset
def moving_part(bp,name,mesh,location):
 part=next((c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name().startswith(name)),None) or ActorTools.add_component(bp,unreal.StaticMeshComponent.static_class(),name)
 body=next(c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='BuildingMesh');assert ActorTools.set_parent_component(part,body)
 part.set_static_mesh(mesh);part.set_editor_property('override_materials',[]);part.set_editor_property('relative_location',location);part.set_editor_property('relative_rotation',unreal.Rotator());part.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));part.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE);part.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
try:
 unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0');reports=[]
 for key,resource,pivot in [('SilicaExtractor',unreal.ResourceType.SILICA,unreal.Vector(104,19,158)),('RareMineralMine',unreal.ResourceType.RARE_MINERALS,unreal.Vector(104,-27,170))]:
  data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_'+key);bp=unreal.load_asset('/Game/Units/Buildings/BlueprintCatalog/'+key+'/BP_'+key+'Building');assert data and bp
  cdo=unreal.get_default_object(bp.generated_class());assert isinstance(cdo,unreal.MiningMachine)
  props=['construction_costs','max_drone_slots','initially_unlocked_drone_slots','energy_consumption_per_minute','energy_production_per_minute','build_tool','blueprint_initially_owned']
  before={p:str(data.get_editor_property(p)) for p in props};rate=cdo.get_output_per_minute_at100_percent(resource)
  meshes={part:import_asset('SM_'+key+'_'+part) for part in ['Refined','Drill','Fan']}
  body=next(c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='BuildingMesh');body.set_static_mesh(meshes['Refined']);body.set_editor_property('override_materials',[]);body.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE)
  # Replace the old unconditional catalogue motion with the shared operational component.
  graph=BlueprintTools.get_graph(bp,'EventGraph')
  for node in list(BlueprintTools.find_nodes(graph)):BlueprintTools.delete_node(node)
  for c in components(bp):
   if isinstance(c,unreal.NiagaraComponent):c.set_editor_property('auto_activate',False);c.set_asset(None)
  moving_part(bp,'MineDrill',meshes['Drill'],unreal.Vector(-80,0,44));moving_part(bp,'Motion',meshes['Fan'],pivot)
  fx=next((c for c in components(bp) if isinstance(c,unreal.IndustrialEffectsComponent)),None) or ActorTools.add_component(bp,unreal.IndustrialEffectsComponent.static_class(),'IndustrialEffects');fx.set_editor_property('effect_kind',unreal.IndustrialEffectKind.SURFACE_MINE)
  cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('ground_mesh_to_surface',True);data.set_editor_property('building_mesh',meshes['Refined'])
  icon=import_asset('T_'+key+'_Refined',True);icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);icon.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);lib.save_loaded_asset(icon);data.set_editor_property('toolbar_icon',icon);data.set_editor_property('thumbnail',icon)
  unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
  data.set_editor_property('building_class',bp.generated_class());lib.save_loaded_asset(bp);lib.save_loaded_asset(data)
  assert before=={p:str(data.get_editor_property(p)) for p in props};assert unreal.get_default_object(bp.generated_class()).get_output_per_minute_at100_percent(resource)==rate
  reports.append({'building':key,'preserved_data':before,'output_per_drone':rate,'bounds':str(meshes['Refined'].get_bounding_box())})
 result={'passed':True,'buildings':reports,'materials':'Shared geothermal detail materials and nuclear status material','effects':'Shared operational drill, fan, dust and night worklights'}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/MineralMinesImport.json').write_text(json.dumps(result,indent=2));unreal.log('MINERAL_MINES_IMPORT '+json.dumps(result))
