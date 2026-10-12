import unreal,json,traceback
from pathlib import Path
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path('C:/UE5/SurviveThePlanet 5.8')
src=root/'ContentSource/WaterCooling';dest='/Game/Units/Buildings/WaterCooling'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def imported(name,texture=False):
 task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_path=dest;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt
 tools.import_asset_tasks([task]);asset=unreal.load_asset(dest+'/'+name);assert asset,name
 if not texture:
  for i,slot in enumerate(asset.get_editor_property('static_materials')):
   key=next(k for k in ['Ivory','Orange','Dark','Steel','Cyan','Lamp'] if 'Geo_'+k in str(slot.material_slot_name))
   mat=unreal.load_asset('/Game/Geothermal/M_GeoDetail_'+key);assert mat,key;asset.set_material(i,mat)
 else:
  asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
 lib.save_loaded_asset(asset);return asset
try:
 fan=imported('SM_ThermalFan');report=[]
 for key,recipe,pivot,kind in [('WaterPurifier',unreal.ThermalRecipe.WATER_PURIFICATION,(0,72,201),unreal.IndustrialEffectKind.WATER_TREATMENT),('CoolingSystemsPlant',unreal.ThermalRecipe.COOLING_UNITS,(0,-30,206),unreal.IndustrialEffectKind.COOLING_PLANT)]:
  data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_'+key)
  bp=unreal.load_asset('/Game/Units/Buildings/BlueprintCatalog/'+key+'/BP_'+key+'Building');assert data and bp,key
  mesh=imported('SM_'+key+'_Refined');icon=imported('T_'+key+'_Refined',True)
  # Remove the catalog's unconditional rotation before native operation-gated animation takes ownership.
  graph=BlueprintTools.get_graph(bp,'EventGraph');BlueprintTools.write_graph_dsl(graph,'')
  unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.ThermalProcessingBuilding)
  cdo=unreal.get_default_object(bp.generated_class())
  for obj in [data,cdo]:
   obj.set_editor_property('max_drone_slots',4);obj.set_editor_property('initially_unlocked_drone_slots',4);obj.set_editor_property('drone_work_type',unreal.STPDroneWorkType.CONSTRUCTION)
  cdo.set_editor_property('recipe',recipe);cdo.set_editor_property('building_data',data);cdo.set_editor_property('ground_mesh_to_surface',True)
  cdo.set_editor_property('primary_input_per_cycle',2);cdo.set_editor_property('purified_water_per_cycle',1);cdo.set_editor_property('output_per_cycle',1);cdo.set_editor_property('cycle_seconds',20)
  data.set_editor_property('override_energy_settings',True);data.set_editor_property('energy_consumption_per_minute',12.0);data.set_editor_property('energy_production_per_minute',0.0)
  data.set_editor_property('building_mesh',mesh);data.set_editor_property('toolbar_icon',icon);data.set_editor_property('thumbnail',icon)
  data.set_editor_property('description','2 Water + grid power -> 1 Purified Water per 20 seconds per 100% drone. Supports 1-4 drones.' if key=='WaterPurifier' else '2 Steel + 1 Purified Water -> 1 Cooling Unit per 20 seconds per 100% drone. Supports 1-4 drones.')
  comps=components(bp);body=next(o for o in comps if isinstance(o,unreal.StaticMeshComponent) and o.get_name()=='BuildingMesh')
  body.set_static_mesh(mesh);body.set_editor_property('override_materials',[]);body.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));body.set_editor_property('relative_location',unreal.Vector(0,0,0))
  for motion in [o for o in comps if isinstance(o,unreal.StaticMeshComponent) and o.get_name().startswith('Motion')]:
   if motion.get_name()=='MotionFan':
    assert ActorTools.set_parent_component(motion,body);motion.set_static_mesh(fan);motion.set_editor_property('override_materials',[]);motion.set_editor_property('relative_location',unreal.Vector(*pivot));motion.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));motion.set_editor_property('mobility',unreal.ComponentMobility.MOVABLE);motion.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
   else:motion.set_static_mesh(None);motion.set_visibility(False)
  for fx in [o for o in comps if isinstance(o,unreal.NiagaraComponent)]:fx.set_editor_property('auto_activate',False);fx.set_asset(None)
  effects=next(o for o in comps if isinstance(o,unreal.IndustrialEffectsComponent));effects.set_editor_property('effect_kind',kind)
  unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
  data.set_editor_property('building_class',bp.generated_class());assert lib.save_loaded_asset(data);assert lib.save_loaded_asset(bp)
  report.append({'key':key,'mesh':mesh.get_path_name(),'recipe':str(recipe),'slots':4,'energy_per_minute':12,'cycle_seconds':20,'status':str(bp.get_editor_property('status'))})
 result={'passed':True,'buildings':report}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/WaterCoolingImport.json').write_text(json.dumps(result,indent=2));unreal.log('WATER_COOLING_IMPORT '+json.dumps(result))
