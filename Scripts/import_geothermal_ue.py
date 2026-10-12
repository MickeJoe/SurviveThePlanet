import unreal,json,traceback
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path('C:/UE5/SurviveThePlanet 5.8')
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools();dest='/Game/Geothermal';src=root/'ContentSource/Geothermal'
def import_asset(name,texture=False):
 task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_path=dest;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt
 tools.import_asset_tasks([task]);a=unreal.load_asset(dest+'/'+name);assert a,name;return a
def material(name):
 m=unreal.load_asset(dest+'/'+name) or tools.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew());return m
edit=unreal.MaterialEditingLibrary
def expr(m,cls):return edit.create_material_expression(m,cls)
def constant(m,value):
 n=expr(m,unreal.MaterialExpressionConstant);n.r=value;return n
def connect(a,out,b,inp):
 assert edit.connect_material_expressions(a,out,b,inp) or (inp=='Input' and edit.connect_material_expressions(a,out,b,'')),(a.get_class().get_name(),b.get_class().get_name(),inp)
def prop(n,m,p,out=''):assert edit.connect_material_property(n,out,p)
try:
 meshes={n:import_asset(n) for n in ['SM_GeothermalPlant','SM_GeothermalRotor','SM_GeothermalSite']}
 colors={'Ivory':(.72,.70,.63),'Orange':(.85,.21,.025),'Dark':(.06,.075,.085),'Steel':(.25,.30,.32),'Cyan':(.02,.65,.9),'Rock':(.09,.075,.06),'Mineral':(.24,.19,.12),'Heat':(1,.095,.004),'Lamp':(1,.75,.36)}
 textures={n:import_asset(n,True) for n in ['T_GeoSurface','T_GeoRock','T_GeoRoughness','T_GeoNormal','T_GeoSteam']}
 for name,texture in textures.items():
  if name in ['T_GeoRoughness','T_GeoNormal']:texture.set_editor_property('srgb',False)
  if name=='T_GeoNormal':texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
  lib.save_loaded_asset(texture)
 def sample(m,name):
  node=expr(m,unreal.MaterialExpressionTextureSample);node.texture=textures[name]
  if name=='T_GeoNormal':node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
  elif name=='T_GeoRoughness':node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
  return node
 materials={}
 for key,color in colors.items():
  existing=unreal.load_asset(dest+'/M_GeoDetail_'+key)
  if existing:
   materials[key]=existing
   continue
  m=material('M_GeoDetail_'+key);c=expr(m,unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(*color,1);surface=sample(m,'T_GeoRock' if key in ['Rock','Mineral'] else 'T_GeoSurface')
  shaded=expr(m,unreal.MaterialExpressionMultiply);connect(c,'',shaded,'A');connect(surface,'RGB',shaded,'B');prop(shaded,m,unreal.MaterialProperty.MP_BASE_COLOR)
  rough=sample(m,'T_GeoRoughness')
  scale=expr(m,unreal.MaterialExpressionMultiply);connect(rough,'R',scale,'A');connect(constant(m,1.4 if key in ['Rock','Mineral'] else .7 if key=='Steel' else 1),'',scale,'B');prop(scale,m,unreal.MaterialProperty.MP_ROUGHNESS)
  prop(sample(m,'T_GeoNormal'),m,unreal.MaterialProperty.MP_NORMAL,'RGB')
  prop(surface,m,unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,'R')
  prop(constant(m,.8 if key=='Steel' else .35 if key=='Dark' else 0),m,unreal.MaterialProperty.MP_METALLIC)
  if key in ['Heat','Cyan','Lamp']:
   scale=constant(m,9.0 if key=='Heat' else 4.0 if key=='Lamp' else 2.5)
   if key=='Heat':
    time=expr(m,unreal.MaterialExpressionTime);sine=expr(m,unreal.MaterialExpressionSine);sine.set_editor_property('period',5);connect(time,'',sine,'Input')
    mul=expr(m,unreal.MaterialExpressionMultiply);connect(sine,'',mul,'A');connect(constant(m,.5),'',mul,'B')
    add=expr(m,unreal.MaterialExpressionAdd);connect(mul,'',add,'A');connect(scale,'',add,'B');scale=add
   emit=expr(m,unreal.MaterialExpressionMultiply);connect(c,'',emit,'A');connect(scale,'',emit,'B');prop(emit,m,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  edit.recompile_material(m);lib.save_loaded_asset(m);materials[key]=m
 for mesh in meshes.values():
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   key=next(k for k in colors if ('Geo_'+k) in str(slot.material_slot_name));mesh.set_material(i,materials[key])
  lib.save_loaded_asset(mesh)
 steam_exists=lib.does_asset_exist(dest+'/M_GeothermalSteamV3')
 steam=material('M_GeothermalSteamV3')
 if not steam_exists:
  steam.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
  steam.set_editor_property('two_sided',True)
  steam.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  vapor=sample(steam,'T_GeoSteam')
  particle=expr(steam,unreal.MaterialExpressionParticleColor)
  color=expr(steam,unreal.MaterialExpressionMultiply);connect(vapor,'RGB',color,'A');connect(particle,'RGB',color,'B');prop(color,steam,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  opacity=expr(steam,unreal.MaterialExpressionMultiply);connect(vapor,'A',opacity,'A');connect(particle,'A',opacity,'B')
  fade=expr(steam,unreal.MaterialExpressionDepthFade);connect(opacity,'',fade,'Opacity');fade.set_editor_property('fade_distance_default',28);prop(fade,steam,unreal.MaterialProperty.MP_OPACITY)
 edit.set_material_usage(steam,unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES);edit.recompile_material(steam);lib.save_loaded_asset(steam)
 icon=import_asset('T_GeothermalPlant',True);icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
 data=unreal.load_asset('/Game/Data/Buildings/DA_GeothermalPlant')
 if not data:
  entries=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog').get_editor_property('buildings')
  data=next(d for d in entries if d.get_editor_property('build_tool')==unreal.STPBuildTool.GEOTHERMAL_PLANT)
 factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.GeothermalPlant)
 bp=unreal.load_asset(dest+'/BP_GeothermalPlant') or tools.create_asset('BP_GeothermalPlant',dest,unreal.Blueprint,factory)
 cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('building_data',data)
 cdo.get_editor_property('effects').set_editor_property('vent_locations',[unreal.Vector(-65,-30,210),unreal.Vector(65,-90,163)])
 data.set_editor_property('building_mesh',meshes['SM_GeothermalPlant']);data.set_editor_property('thumbnail',icon);data.set_editor_property('toolbar_icon',icon)
 # Keep existing balance, costs, drone slots and availability.
 for component in cdo.get_components_by_class(unreal.StaticMeshComponent):
  if component.get_name()=='BuildingMesh':component.set_static_mesh(meshes['SM_GeothermalPlant'])
  elif component.get_name()=='TurbineRotor':
   component.set_static_mesh(meshes['SM_GeothermalRotor']);component.set_relative_location(unreal.Vector(74,65,57),False,False)
 # Grid footprint is derived from the 280 x 260 cm mesh bounds.
 site_cdo=unreal.get_default_object(unreal.GeothermalSite)
 site_cdo.get_editor_property('site_mesh').set_static_mesh(meshes['SM_GeothermalSite'])
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);data.set_editor_property('building_class',bp.generated_class());lib.save_loaded_asset(bp);lib.save_loaded_asset(data)
 planets=lib.list_assets('/Game/Data/Planet',recursive=True,include_folder=False)
 aridus=next(unreal.load_asset(p) for p in planets if 'Aridus' in p and isinstance(unreal.load_asset(p),unreal.PlanetDefinition))
 aridus.set_editor_property('geothermal_site_frequency',.28);aridus.set_editor_property('guarantee_starting_geothermal_site',True);lib.save_loaded_asset(aridus)
 report={'passed':True,'data':data.get_path_name(),'planet':aridus.get_path_name(),'frequency':.28,'footprint':str(cdo.get_grid_footprint()),'meshes':{k:str(v.get_bounding_box()) for k,v in meshes.items()}}
except Exception:
 report={'passed':False,'error':traceback.format_exc()}
(root/'Saved/GeothermalImport.json').write_text(json.dumps(report,indent=2))
unreal.log('GEOTHERMAL_IMPORT '+json.dumps(report))
