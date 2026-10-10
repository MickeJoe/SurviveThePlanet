"""Import the Blender building catalog into the canonical UE 5.8 project.
Run through the native MCP-controlled editor Python console.
"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
assert ROOT==Path(r'C:\UE5\SurviveThePlanet 5.8')
OUTPUT=Path(r'C:\Users\qtxmj\Documents\Codex\2026-10-07\kan-du-skapa-alla-meshes-f-2\outputs')
SRC=OUTPUT/'BuildingCatalog'
DEST='/Game/Units/Buildings/BlueprintCatalog'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary;mel=unreal.MaterialEditingLibrary
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
def node(m,c,x=0,y=0):return mel.create_material_expression(m,c,x,y)
def prop(n,p):assert mel.connect_material_property(n,'',p)
def link(a,b,pin,output=''):assert mel.connect_material_expressions(a,output,b,pin)
def color(m,rgb):
 n=node(m,unreal.MaterialExpressionConstant3Vector,-700,0);n.constant=unreal.LinearColor(*rgb,1);return n
def value(m,f):
 n=node(m,unreal.MaterialExpressionConstant,-700,200);n.r=f;return n
palette={'Ivory':(.66,.64,.57),'Graphite':(.045,.059,.065),'Steel':(.27,.32,.34),'Orange':(.8,.17,.028),'Cyan':(.01,.65,.9),'Glass':(.025,.12,.16),'Heat':(1,.22,.018)}
materials={}
for name,rgb in palette.items():
 path=DEST+'/Materials/M_Catalog_'+name;m=unreal.load_asset(path)
 if not m:m=tools.create_asset('M_Catalog_'+name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
 if lib.get_metadata_tag(m,'CatalogComplete')!='3':
  mel.delete_all_material_expressions(m)
  if name in ('Cyan','Heat'):
   c=color(m,rgb);prop(c,unreal.MaterialProperty.MP_BASE_COLOR)
   time=node(m,unreal.MaterialExpressionTime,-700,300)
   sine=node(m,unreal.MaterialExpressionSine,-500,300);sine.set_editor_property('period',3 if name=='Cyan' else 1.8);link(time,sine,'')
   mult=node(m,unreal.MaterialExpressionMultiply,-300,300);mult.set_editor_property('const_b',.7);link(sine,mult,'A')
   bias=node(m,unreal.MaterialExpressionAdd,-100,300);bias.set_editor_property('const_b',2.2);link(mult,bias,'A')
   emission=node(m,unreal.MaterialExpressionMultiply,100,0);link(c,emission,'A');link(bias,emission,'B');prop(emission,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  else:
   noise=node(m,unreal.MaterialExpressionNoise,-650,-200);noise.set_editor_property('scale',.005);noise.set_editor_property('output_min',0.0);noise.set_editor_property('output_max',1.0)
   mix=node(m,unreal.MaterialExpressionLinearInterpolate,-150,0)
   link(color(m,tuple(v*.85 for v in rgb)),mix,'A');link(color(m,rgb),mix,'B');link(noise,mix,'Alpha');prop(mix,unreal.MaterialProperty.MP_BASE_COLOR)
  prop(value(m,.55 if name in ('Steel','Graphite') else .15),unreal.MaterialProperty.MP_METALLIC)
  prop(value(m,.62),unreal.MaterialProperty.MP_ROUGHNESS)
  mel.recompile_material(m);lib.set_metadata_tag(m,'CatalogComplete','3');assert lib.save_loaded_asset(m)
 materials['Catalog_'+name]=m
records=json.loads((SRC/'authored_manifest.json').read_text())
reports=[]
for i in records[globals().get('BATCH_START',0):globals().get('BATCH_END',49)]:
 key=i['key'];destination=DEST+'/'+key;meshes={}
 for part in ('Body','Motion'):
  name='SM_'+key+'_'+part;path=destination+'/Meshes/'+name
  mesh=unreal.load_asset(path)
  scale_stamp='legacy_cm_4' if part=='Motion' else 'legacy_cm_3'
  if not mesh or lib.get_metadata_tag(mesh,'CatalogScale')!=scale_stamp:
   task=unreal.AssetImportTask();task.filename=str(SRC/key/(name+'.fbx'));task.destination_path=destination+'/Meshes';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
   task.factory=unreal.FbxFactory();task.replace_existing_settings=True
   opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
   opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=part=='Body';opts.static_mesh_import_data.generate_lightmap_u_vs=True
   opts.static_mesh_import_data.transform_vertex_to_absolute=False;opts.static_mesh_import_data.bake_pivot_in_vertex=False
   opts.static_mesh_import_data.convert_scene_unit=False;opts.static_mesh_import_data.import_uniform_scale=100.0
   task.options=opts;tools.import_asset_tasks([task]);mesh=unreal.load_asset(path)
  assert mesh,name
  for idx,slot in enumerate(mesh.get_editor_property('static_materials')):
   slotname=str(slot.material_slot_name)
   match=next((mat for prefix,mat in materials.items() if slotname.startswith(prefix)),materials['Catalog_Ivory'])
   mesh.set_material(idx,match)
  settings=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_lod_build_settings(mesh,0)
  settings.set_editor_property('build_scale3d',unreal.Vector(1,1,1))
  unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_lod_build_settings(mesh,0,settings)
  lib.set_metadata_tag(mesh,'CatalogScale',scale_stamp);assert lib.save_loaded_asset(mesh);meshes[part]=mesh
 path=destination+'/BP_'+key
 bp=unreal.load_asset(path)
 if not bp:
  factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.StaticMeshActor)
  bp=tools.create_asset('BP_'+key,destination,unreal.Blueprint,factory)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp)
 cdo=unreal.get_default_object(bp.generated_class());cdo.static_mesh_component.set_static_mesh(meshes['Body']);cdo.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 cdo.set_editor_property('tags',[unreal.Name('BuildingCatalogVisual'),unreal.Name('BP%02d'%i['id'])])
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert lib.save_loaded_asset(bp)
 reports.append({'key':key,'blueprint':path,'body':meshes['Body'].get_path_name(),'motion':meshes['Motion'].get_path_name(),'extent':str(meshes['Body'].get_bounding_box())})
 (OUTPUT/('ue_import_batch_%02d.json'%globals().get('BATCH_START',0))).write_text(json.dumps(reports,indent=2))
unreal.log('BUILDING_CATALOG_IMPORT_COMPLETED '+str(len(reports)))
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 1')
