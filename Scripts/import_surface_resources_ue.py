"""Import Blender deposits, configure extraction, and tune planet occurrence."""
import unreal,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path('C:/UE5/SurviveThePlanet 5.8')
dest='/Game/Resources/SurfaceDeposits';src=root/'ContentSource/SurfaceResources'
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
def components(bp):return [unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)) for h in sub.k2_gather_subobject_data_for_blueprint(bp)]
def import_asset(name,texture=False,destination=None):
 task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_name=name;task.destination_path=destination or dest;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opt=unreal.FbxImportUI();opt.import_mesh=True;opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.auto_generate_collision=True;task.options=opt
 tools.import_asset_tasks([task]);asset=unreal.load_asset(task.destination_path+'/'+name);assert asset;return asset
def shared_material():
 name='M_DepositSurface';m=unreal.load_asset(dest+'/'+name)
 if m and lib.get_metadata_tag(m,'SurfaceDepositVersion')=='2':return m
 m=m or tools.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew());edit.delete_all_material_expressions(m)
 def node(cls):return edit.create_material_expression(m,cls)
 def link(a,out,b,pin):assert edit.connect_material_expressions(a,out,b,pin)
 def scalar(name,value):
  n=node(unreal.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value);return n
 color=node(unreal.MaterialExpressionVectorParameter);color.set_editor_property('parameter_name','SurfaceColor');color.set_editor_property('default_value',unreal.LinearColor(.4,.35,.28,1))
 tex=node(unreal.MaterialExpressionTextureSample);tex.texture=unreal.load_asset('/Game/Geothermal/T_GeoRock');assert tex.texture
 variation=node(unreal.MaterialExpressionLinearInterpolate);white=node(unreal.MaterialExpressionConstant3Vector);white.constant=unreal.LinearColor(1,1,1,1);link(white,'',variation,'A');link(tex,'RGB',variation,'B');link(scalar('TextureInfluence',1),'',variation,'Alpha')
 tint=node(unreal.MaterialExpressionMultiply);link(color,'',tint,'A');link(variation,'',tint,'B');assert edit.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
 roughtex=node(unreal.MaterialExpressionTextureSample);roughtex.texture=unreal.load_asset('/Game/Geothermal/T_GeoRoughness');roughtex.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
 rough=node(unreal.MaterialExpressionMultiply);link(roughtex,'R',rough,'A');link(scalar('Roughness',.75),'',rough,'B');edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 edit.connect_material_property(scalar('Metallic',0),'',unreal.MaterialProperty.MP_METALLIC)
 normal=node(unreal.MaterialExpressionTextureSample);normal.texture=unreal.load_asset('/Game/Geothermal/T_GeoNormal');normal.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
 flatten=node(unreal.MaterialExpressionLinearInterpolate);link(normal,'RGB',flatten,'A')
 flat=node(unreal.MaterialExpressionConstant3Vector);flat.constant=unreal.LinearColor(0,0,1,1);link(flat,'',flatten,'B');link(scalar('NormalFlatness',.1),'',flatten,'Alpha');edit.connect_material_property(flatten,'',unreal.MaterialProperty.MP_NORMAL)
 ao=node(unreal.MaterialExpressionLinearInterpolate);link(white,'',ao,'A');link(tex,'R',ao,'B');link(scalar('TextureInfluence',1),'',ao,'Alpha');edit.connect_material_property(ao,'',unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
 time=node(unreal.MaterialExpressionTime);sine=node(unreal.MaterialExpressionSine);sine.set_editor_property('period',6);link(time,'',sine,'')
 pulse=node(unreal.MaterialExpressionMultiply);link(sine,'',pulse,'A');link(scalar('PulseAmount',0),'',pulse,'B')
 bias=node(unreal.MaterialExpressionAdd);bias.set_editor_property('const_b',1);link(pulse,'',bias,'A')
 glow=node(unreal.MaterialExpressionMultiply);link(color,'',glow,'A');link(scalar('Emission',0),'',glow,'B')
 emission=node(unreal.MaterialExpressionMultiply);link(glow,'',emission,'A');link(bias,'',emission,'B');edit.connect_material_property(emission,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 edit.recompile_material(m);lib.set_metadata_tag(m,'SurfaceDepositVersion','2');lib.save_loaded_asset(m);return m
def instances(parent):
 palette={'PaleRock':((.65,.52,.34),.95,.0,0,.1),'Quartz':((.94,1,.94),.43,.02,0,.85),'QuartzShadow':((.44,.57,.59),.50,.05,0,.85),
 'RareRock':((.14,.19,.26),.90,.10,0,.1),'Amethyst':((.38,.055,.62),.45,.28,.06,.70),'RareVein':((.31,.025,.53),.55,.20,.18,.6),
 'MetalFleck':((.53,.45,.57),.48,.75,0,.2),'Obsidian':((.045,.069,.070),.70,.22,0,.2),'AlienShell':((.11,.17,.135),.82,.15,0,.15),
 'AlienCore':((.018,.48,.40),.46,.13,.22,.7),'AlienVein':((.012,.32,.28),.52,.08,.75,.55)}
 result={}
 for key,(rgb,rough,metal,emission,flat) in palette.items():
  name='MI_Deposit_'+key;mi=unreal.load_asset(dest+'/'+name) or tools.create_asset(name,dest,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew());edit.set_material_instance_parent(mi,parent)
  edit.set_material_instance_vector_parameter_value(mi,'SurfaceColor',unreal.LinearColor(*rgb,1))
  influence=.25 if key.startswith('Quartz') else (.65 if key in ['Amethyst','AlienCore'] else 1)
  for n,v in [('Roughness',rough),('Metallic',metal),('Emission',emission),('NormalFlatness',flat),('TextureInfluence',influence),('PulseAmount',.12 if key.startswith('Alien') else 0)]:edit.set_material_instance_scalar_parameter_value(mi,n,v)
  lib.save_loaded_asset(mi);result['Deposit_'+key]=mi
 return result
def configure_mine(key,resource):
 bp=unreal.load_asset('/Game/Units/Buildings/BlueprintCatalog/'+key+'/BP_'+key+'Building');data=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_'+key);assert bp and data
 if not isinstance(unreal.get_default_object(bp.generated_class()),unreal.MiningMachine):unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.MiningMachine)
 cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('building_data',data);cdo.set_editor_property('supported_resource_types',[resource]);cdo.set_editor_property('output_per_drone_at100_percent',[unreal.STPResourceOutputRate(resource=resource,amount_per_minute_per_drone_at100_percent=10)])
 cdo.set_editor_property('ground_mesh_to_surface',True);cdo.set_editor_property('building_type',unreal.STPBuildingType.MINING_MACHINE);cdo.set_editor_property('drone_work_type',unreal.STPDroneWorkType.MINING)
 data.set_editor_property('building_type',unreal.STPBuildingType.MINING_MACHINE);data.set_editor_property('drone_work_type',unreal.STPDroneWorkType.MINING)
 reference=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CoalMine');assert reference
 for name in ['max_drone_slots','initially_unlocked_drone_slots']:data.set_editor_property(name,reference.get_editor_property(name))
 data.set_editor_property('override_energy_settings',True);data.set_editor_property('energy_consumption_per_minute',10);data.set_editor_property('energy_production_per_minute',0)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);data.set_editor_property('building_class',bp.generated_class());lib.save_loaded_asset(bp);lib.save_loaded_asset(data);return bp
try:
 unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
 materials=instances(shared_material());report=[];classes={}
 for key,resource,minekey in [('Silica',unreal.ResourceType.SILICA,'SilicaExtractor'),('RareMinerals',unreal.ResourceType.RARE_MINERALS,'RareMineralMine'),('AlienMaterial',unreal.ResourceType.ALIEN_MATERIAL,'AlienMaterialHarvester')]:
  mesh=import_asset('SM_'+key+'Deposit')
  for i,slot in enumerate(mesh.static_materials):
   material=next((v for n,v in materials.items() if str(slot.material_slot_name)==n or str(slot.material_slot_name).startswith(n+'.')),None);assert material,str(slot.material_slot_name);mesh.set_material(i,material)
  lib.save_loaded_asset(mesh);mine=configure_mine(minekey,resource)
  name='BP_'+key+'Deposit';bp=unreal.load_asset(dest+'/'+name)
  if not bp:
   factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.BaseResourceSource);bp=tools.create_asset(name,dest,unreal.Blueprint,factory)
  cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('resource_type',resource);cdo.set_editor_property('starting_amount',4000);cdo.set_editor_property('mine_blueprint',mine.generated_class());cdo.set_editor_property('remains_visible_after_discovery',True)
  body=next(c for c in components(bp) if isinstance(c,unreal.StaticMeshComponent) and c.get_name()=='ResourceMesh');body.set_static_mesh(mesh);body.set_editor_property('override_materials',[])
  unreal.BlueprintEditorLibrary.compile_blueprint(bp);lib.save_loaded_asset(bp);classes[key]=bp.generated_class()
  icon=import_asset('T_'+key,True,'/Game/UI/Icons/Resources');icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);icon.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);lib.save_loaded_asset(icon)
  report.append({'resource':key,'mesh':mesh.get_path_name(),'mine':mine.get_path_name(),'bounds':str(mesh.get_bounding_box())})
 planets=[]
 for path in lib.list_assets('/Game/Data/Planet',True,False):
  planet=unreal.load_asset(path)
  if not isinstance(planet,unreal.PlanetDefinition):continue
  settings=[];name=planet.get_name()
  for key,resource,freq,offset in [('Silica',unreal.ResourceType.SILICA,.65,unreal.Vector(-850,650,0)),('RareMinerals',unreal.ResourceType.RARE_MINERALS,.20,unreal.Vector(-850,-650,0)),('AlienMaterial',unreal.ResourceType.ALIEN_MATERIAL,.14,unreal.Vector(200,-1050,0))]:
   if key=='RareMinerals' and 'Nexaris' not in name:continue
   if key=='AlienMaterial' and 'Aridus' not in name:continue
   rule=unreal.PlanetResourceRule(id=key,resource_type=resource,min_quantity=3000,max_quantity=5000,radius=150,minimum_spacing=450)
   settings.append(unreal.PlanetSurfaceDepositSettings(deposit=rule,source_class=classes[key],sector_frequency=freq,guarantee_starting_deposit=True,starting_offset=offset))
  planet.set_editor_property('surface_deposits',settings);lib.save_loaded_asset(planet);planets.append({'planet':name,'resources':[str(s.deposit.resource_type) for s in settings]})
 result={'passed':True,'deposits':report,'planets':planets,'shared_material_parent':dest+'/M_DepositSurface'}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/SurfaceResourcesImport.json').write_text(json.dumps(result,indent=2));unreal.log('SURFACE_RESOURCES_IMPORT '+json.dumps(result))
