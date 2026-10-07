import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
src=root/"ContentSource/ConnectorFactory";dest="/Game/Units/Buildings/ConnectorFactory"
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
body=unreal.load_asset(dest+"/Meshes/SM_ConnectorFactory")
old_materials=[slot.material_interface for slot in body.get_editor_property("static_materials")]
meshes={}
for name in ["SM_ConnectorFactory","SM_ConnectorFanA","SM_ConnectorFanB","SM_ConnectorProduct"]:
 task=unreal.AssetImportTask();task.filename=str(src/(name+".fbx"));task.destination_path=dest+"/Meshes";task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
 opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
 opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=True;opts.static_mesh_import_data.generate_lightmap_u_vs=True;task.options=opts
 tools.import_asset_tasks([task]);mesh=unreal.load_asset(dest+"/Meshes/"+name);assert mesh;meshes[name]=mesh
 for i,slot in enumerate(mesh.get_editor_property("static_materials")):
  mesh.set_material(i,old_materials[i] if i<len(old_materials) else old_materials[0])
 assert lib.save_loaded_asset(mesh)
material=unreal.load_asset(dest+"/Materials/M_ConnectorWeldSpark") or tools.create_asset("M_ConnectorWeldSpark",dest+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
color=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-250,0)
color.constant=unreal.LinearColor(35,10,1,1)
unreal.MaterialEditingLibrary.connect_material_property(color,"",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
material.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_UNLIT)
unreal.MaterialEditingLibrary.recompile_material(material);assert lib.save_loaded_asset(material)
bp=unreal.load_asset("/Game/BluePrints/Buildings/ConnectorFactory/BP_ConnectorFactory")
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=unreal.get_default_object(bp.generated_class())
for name,value in {"fan_mesh_a":meshes["SM_ConnectorFanA"],"fan_mesh_b":meshes["SM_ConnectorFanB"],"product_mesh":meshes["SM_ConnectorProduct"],"spark_material":material}.items():cdo.set_editor_property(name,value)
unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert lib.save_loaded_asset(bp)
(root/"Saved/ConnectorAnimationImport.json").write_text(json.dumps({"passed":True,"meshes":{k:str(v.get_bounding_box()) for k,v in meshes.items()}},indent=2))
unreal.log("CONNECTOR_ANIMATION_IMPORTED")

