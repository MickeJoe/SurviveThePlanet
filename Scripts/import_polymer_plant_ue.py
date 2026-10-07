import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r"C:\UE5\SurviveThePlanet 5.8"),root
src=root/"ContentSource/PolymerPlant";dest="/Game/Units/Buildings/PolymerPlant"
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
def imported(name,folder,texture=False):
 existing=unreal.load_asset(folder+"/"+name)
 # Reimport changed source geometry/textures without rebuilding existing materials.
 task=unreal.AssetImportTask();task.filename=str(src/(name+(".png" if texture else ".fbx")))
 task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 if not texture:
  opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
  opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=True
  opts.static_mesh_import_data.generate_lightmap_u_vs=True;task.options=opts
 tools.import_asset_tasks([task]);asset=unreal.load_asset(folder+"/"+name);assert asset,name;return asset
meshes={n:imported(n,dest+"/Meshes") for n in ["SM_PolymerPlant","SM_PolymerFan","SM_PolymerBlock"]}
base_color=imported("T_PolymerPlant_BaseColor",dest+"/Textures",True)
ao=imported("T_PolymerPlant_AO",dest+"/Textures",True)
ao.set_editor_property("srgb",False);lib.save_loaded_asset(ao)
icon=imported("T_PolymerPlant","/Game/UI/Icons/Buildings",True)
icon.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON)
icon.set_editor_property("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
palette={"Polymer_Ivory":((.72,.70,.64),.25,.45),"Polymer_Orange":((.9,.19,.025),.25,.45),"Polymer_Graphite":((.055,.075,.09),.55,.45),"Polymer_Steel":((.28,.34,.37),.7,.45),"Polymer_Cyan":((.01,.65,.9),.1,.4),"Polymer_Product":((.85,.86,.78),0,.45)}
materials={}
for key,(color,metal,rough) in palette.items():
 m=unreal.load_asset(dest+"/Materials/M_"+key)
 if m:
  materials[key]=m
  continue
 m=tools.create_asset("M_"+key,dest+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
 unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
 c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-300,0);c.constant=unreal.LinearColor(*color,1)
 unreal.MaterialEditingLibrary.connect_material_property(c,"",unreal.MaterialProperty.MP_BASE_COLOR)
 for val,prop,y in [(metal,unreal.MaterialProperty.MP_METALLIC,100),(rough,unreal.MaterialProperty.MP_ROUGHNESS,200)]:
  n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-300,y);n.r=val
  unreal.MaterialEditingLibrary.connect_material_property(n,"",prop)
 if key=="Polymer_Cyan":unreal.MaterialEditingLibrary.connect_material_property(c,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 unreal.MaterialEditingLibrary.recompile_material(m);lib.save_loaded_asset(m);materials[key]=m
surface=unreal.load_asset(dest+"/Materials/M_PolymerSurface_v2")
if not surface:
 surface=tools.create_asset("M_PolymerSurface_v2",dest+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
 color_node=unreal.MaterialEditingLibrary.create_material_expression(surface,unreal.MaterialExpressionTextureSample,-400,0);color_node.texture=base_color
 unreal.MaterialEditingLibrary.connect_material_property(color_node,"RGB",unreal.MaterialProperty.MP_BASE_COLOR)
 ao_node=unreal.MaterialEditingLibrary.create_material_expression(surface,unreal.MaterialExpressionTextureSample,-400,220);ao_node.texture=ao;ao_node.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
 unreal.MaterialEditingLibrary.connect_material_property(ao_node,"R",unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
 for value,prop,y in [(.60,unreal.MaterialProperty.MP_ROUGHNESS,420),(.06,unreal.MaterialProperty.MP_METALLIC,520)]:
  node=unreal.MaterialEditingLibrary.create_material_expression(surface,unreal.MaterialExpressionConstant,-400,y);node.r=value
  unreal.MaterialEditingLibrary.connect_material_property(node,"",prop)
 unreal.MaterialEditingLibrary.recompile_material(surface);lib.save_loaded_asset(surface)
for mesh in meshes.values():
 for i,slot in enumerate(mesh.get_editor_property("static_materials")):
  key=next((k for k in palette if k in str(slot.material_slot_name)),None)
  assert key,str(slot.material_slot_name)
  mesh.set_material(i,surface if mesh==meshes["SM_PolymerPlant"] and key!="Polymer_Cyan" else materials[key])
 lib.save_loaded_asset(mesh)
data=unreal.load_asset("/Game/Data/Buildings/DA_PolymerPlant") or tools.create_asset("DA_PolymerPlant","/Game/Data/Buildings",unreal.PolymerPlantBuildingDataAsset,unreal.DataAssetFactory())
data.set_editor_property("building_mesh",meshes["SM_PolymerPlant"]);data.set_editor_property("thumbnail",icon);data.set_editor_property("toolbar_icon",icon)
data.set_editor_property("toolbar_sort_order",35);data.set_editor_property("blueprint_initially_owned",True);data.set_editor_property("show_in_build_toolbar",True)
data.set_editor_property("description","Consumes 2 coal and 1 water every 20 seconds to produce 3 polymer. Requires 12 electricity/minute.")
costs=[]
for resource,amount in [(unreal.ResourceType.IRON,30),(unreal.ResourceType.CONCRETE,10),(unreal.ResourceType.CONTROL_CHIP,2)]:
 c=unreal.ResourceCost();c.resource=resource;c.cost=amount;costs.append(c)
data.set_editor_property("construction_costs",costs)
factory=unreal.BlueprintFactory();factory.set_editor_property("parent_class",unreal.PolymerPlant)
bp=unreal.load_asset("/Game/BluePrints/Buildings/PolymerPlant/BP_PolymerPlant") or tools.create_asset("BP_PolymerPlant","/Game/BluePrints/Buildings/PolymerPlant",unreal.Blueprint,factory)
cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property("building_data",data)
cdo.set_editor_property("fan_mesh_asset",meshes["SM_PolymerFan"]);cdo.set_editor_property("product_mesh_asset",meshes["SM_PolymerBlock"])
for c in cdo.get_components_by_class(unreal.StaticMeshComponent):
 if c.get_name()=="BuildingMesh":
  c.set_static_mesh(meshes["SM_PolymerPlant"])
  c.set_editor_property("relative_location",unreal.Vector(0,0,-56.55))
 elif c.get_name() in ["FanA","FanB"]:c.set_static_mesh(meshes["SM_PolymerFan"])
 elif c.get_name()=="OutputBlock":c.set_static_mesh(meshes["SM_PolymerBlock"])
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
data.set_editor_property("building_class",bp.generated_class());lib.save_loaded_asset(data);lib.save_loaded_asset(bp)
catalog=unreal.load_asset("/Game/Data/Buildings/DA_BuildingCatalog");assert catalog
entries=list(catalog.get_editor_property("buildings"))
if data not in entries:entries.append(data)
catalog.set_editor_property("buildings",entries);lib.save_loaded_asset(catalog)
report={"passed":True,"blueprint":bp.get_path_name(),"data":data.get_path_name(),"catalog":catalog.get_path_name(),"bounds":{k:str(v.get_bounding_box()) for k,v in meshes.items()}}
(root/"Saved/PolymerImport.json").write_text(json.dumps(report,indent=2))
unreal.log("POLYMER_IMPORT_COMPLETE "+json.dumps(report))

