import unreal,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path('C:/UE5/SurviveThePlanet 5.8');dest='/Game/Uranium'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):
 return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def blueprint(name,parent):
 f=unreal.BlueprintFactory();f.set_editor_property('parent_class',parent)
 return unreal.load_asset(dest+'/'+name) or tools.create_asset(name,dest,unreal.Blueprint,f)
def material(name,color,rock=False):
 m=unreal.load_asset(dest+'/'+name)
 if m and 'Activity' in [str(n) for n in edit.get_scalar_parameter_names(m)]:return m
 m=m or tools.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew())
 def node(cls):return edit.create_material_expression(m,cls)
 def link(a,out,b,inp):assert edit.connect_material_expressions(a,out,b,inp)
 c=node(unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(*color,1)
 tex=node(unreal.MaterialExpressionTextureSample);tex.texture=unreal.load_asset('/Game/Geothermal/T_GeoRock' if rock else '/Game/Geothermal/T_GeoSurface')
 mul=node(unreal.MaterialExpressionMultiply);link(c,'',mul,'A');link(tex,'RGB',mul,'B');edit.connect_material_property(mul,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=node(unreal.MaterialExpressionTextureSample);rough.texture=unreal.load_asset('/Game/Geothermal/T_GeoRoughness');rough.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR;edit.connect_material_property(rough,'R',unreal.MaterialProperty.MP_ROUGHNESS)
 normal=node(unreal.MaterialExpressionTextureSample);normal.texture=unreal.load_asset('/Game/Geothermal/T_GeoNormal');normal.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL;edit.connect_material_property(normal,'RGB',unreal.MaterialProperty.MP_NORMAL)
 edit.connect_material_property(tex,'R',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
 activity=node(unreal.MaterialExpressionScalarParameter);activity.set_editor_property('parameter_name','Activity');activity.set_editor_property('default_value',.15)
 glow=node(unreal.MaterialExpressionMultiply);link(mul,'',glow,'A');link(activity,'',glow,'B');edit.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 edit.recompile_material(m);lib.save_loaded_asset(m);return m
try:
 mineral=material('M_UraniumMineral',(.28,.43,.065),True);status=material('M_NuclearStatus',(.02,.65,.9))
 task=unreal.AssetImportTask();task.filename=str(root/'ContentSource/Uranium/SM_UraniumSurface.fbx');task.destination_path=dest;task.destination_name='SM_UraniumSurface';task.automated=True;task.save=True;task.replace_existing=True
 opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt;tools.import_asset_tasks([task])
 mesh=unreal.load_asset(dest+'/SM_UraniumSurface');assert mesh
 for i,s in enumerate(mesh.static_materials):mesh.set_material(i,mineral if 'Mineral' in str(s.material_slot_name) else unreal.load_asset('/Game/Geothermal/M_GeoDetail_Rock'))
 lib.save_loaded_asset(mesh)
 mine=blueprint('BP_UraniumMine',unreal.MiningMachine);mcdo=unreal.get_default_object(mine.generated_class());data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_UraniumMine');assert data
 mcdo.set_editor_property('building_data',data);mcdo.set_editor_property('supported_resource_types',[unreal.ResourceType.URANIUM_ORE]);mcdo.set_editor_property('output_per_drone_at100_percent',[unreal.STPResourceOutputRate(resource=unreal.ResourceType.URANIUM_ORE,amount_per_minute_per_drone_at100_percent=10)])
 mcdo.set_editor_property('ground_mesh_to_surface',True)
 data.set_editor_property('building_type',unreal.STPBuildingType.MINING_MACHINE);data.set_editor_property('drone_work_type',unreal.STPDroneWorkType.MINING)
 if data.get_editor_property('max_drone_slots')==0:
  reference=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CoalMine');assert reference
  for property_name in ['max_drone_slots','initially_unlocked_drone_slots']:data.set_editor_property(property_name,reference.get_editor_property(property_name))
  assert data.get_editor_property('max_drone_slots')>0,'Existing mine must have drone slots'
 body=next(c for c in components(mine) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='BuildingMesh');body.set_static_mesh(data.get_editor_property('building_mesh'))
 unreal.BlueprintEditorLibrary.compile_blueprint(mine);lib.save_loaded_asset(mine)
 data.set_editor_property('building_class',mine.generated_class());lib.save_loaded_asset(data)
 resource=blueprint('BP_UraniumResource',unreal.UraniumResourceSource);cdo=unreal.get_default_object(resource.generated_class());cdo.set_editor_property('mine_blueprint',mine.generated_class())
 next(c for c in components(resource) if isinstance(c,unreal.StaticMeshComponent)).set_static_mesh(mesh)
 unreal.BlueprintEditorLibrary.compile_blueprint(resource);lib.save_loaded_asset(resource)
 for bp_path,kind,mesh_name in [('/Game/BluePrints/Buildings/STP/BP_STP_NuclearReactor',unreal.IndustrialEffectKind.NUCLEAR_REACTOR,'SM_NuclearReactor_Refined'),('/Game/Units/Buildings/BlueprintCatalog/UraniumProcessor/BP_UraniumProcessorBuilding',unreal.IndustrialEffectKind.URANIUM_PROCESSOR,'SM_UraniumProcessor_Refined')]:
  bp=unreal.load_asset(bp_path);effect=next((c for c in components(bp) if isinstance(c,unreal.IndustrialEffectsComponent)),None) or ActorTools.add_component(bp,unreal.IndustrialEffectsComponent.static_class(),'IndustrialEffects');effect.set_editor_property('effect_kind',kind)
  art=unreal.load_asset('/Game/Units/Buildings/NuclearIndustrial/'+mesh_name)
  for i,s in enumerate(art.static_materials):
   if 'Cyan' in str(s.material_slot_name):art.set_material(i,status)
  lib.save_loaded_asset(art)
  if kind==unreal.IndustrialEffectKind.URANIUM_PROCESSOR:
   graph=BlueprintTools.get_graph(bp,'EventGraph')
   BlueprintTools.write_graph_dsl(graph,'')
  unreal.BlueprintEditorLibrary.compile_blueprint(bp);lib.save_loaded_asset(bp)
 planets=[]
 for path in lib.list_assets('/Game/Data/Planet',recursive=True,include_folder=False):
  planet=unreal.load_asset(path)
  if not isinstance(planet,unreal.PlanetDefinition):continue
  borealis='Borealis' in planet.get_name();planet.set_editor_property('uranium_deposit_frequency',.28 if borealis else 0);planet.set_editor_property('guarantee_starting_uranium_deposit',borealis);planet.set_editor_property('uranium_deposit_min_quantity',3000);planet.set_editor_property('uranium_deposit_max_quantity',5000);lib.save_loaded_asset(planet);planets.append({'name':planet.get_name(),'frequency':planet.uranium_deposit_frequency,'starting':borealis})
 if not lib.does_asset_exist(dest+'/NS_UraniumDust'):assert lib.duplicate_asset('/Game/Geothermal/NS_GeothermalSteam',dest+'/NS_UraniumDust')
 result={'passed':True,'planets':planets,'resource':resource.get_path_name(),'mine':mine.get_path_name(),'bounds':str(mesh.get_bounding_box())}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/UraniumImport.json').write_text(json.dumps(result,indent=2));unreal.log('URANIUM_IMPORT '+json.dumps(result))
