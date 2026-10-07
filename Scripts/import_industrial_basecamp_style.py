"""Reimport visual assets only; keep building logic, Blueprint defaults and placement intact."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
report={}
for kind,mesh_name,old_mat in [("PolymerPlant","SM_PolymerPlant","M_PolymerSurface_v2"),("ConnectorFactory","SM_ConnectorFactory","M_ConnectorSurface")]:
    src=root/"ContentSource"/kind;dest="/Game/Units/Buildings/"+kind
    mesh=unreal.load_asset(dest+"/Meshes/"+mesh_name);assert mesh
    old_bounds=mesh.get_bounding_box()
    def imported(name,folder,texture=False):
        task=unreal.AssetImportTask();task.filename=str(src/(name+(".png" if texture else ".fbx")));task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
        if not texture:
            opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
            opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=True;opts.static_mesh_import_data.generate_lightmap_u_vs=True
            task.options=opts
        tools.import_asset_tasks([task]);asset=unreal.load_asset(folder+"/"+name);assert asset;return asset
    mesh=imported(mesh_name,dest+"/Meshes")
    textures={}
    for suffix in ["BaseColor","AO","Roughness","Normal"]:
        tex=imported("T_"+kind+"_"+suffix,dest+"/Textures",True)
        if suffix!="BaseColor":tex.set_editor_property("srgb",False)
        if suffix=="Normal":tex.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_NORMALMAP)
        lib.save_loaded_asset(tex);textures[suffix]=tex
    icon=imported("T_"+kind,"/Game/UI/Icons/Buildings",True)
    icon.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_EDITOR_ICON);icon.set_editor_property("lod_group",unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(icon)
    name="M_"+kind+"_Basecamp"
    material=unreal.load_asset(dest+"/Materials/"+name) or tools.create_asset(name,dest+"/Materials",unreal.Material,unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    for i,(suffix,prop,channel,sampler) in enumerate([
        ("BaseColor",unreal.MaterialProperty.MP_BASE_COLOR,"RGB",unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
        ("AO",unreal.MaterialProperty.MP_AMBIENT_OCCLUSION,"R",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
        ("Roughness",unreal.MaterialProperty.MP_ROUGHNESS,"R",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR),
        ("Normal",unreal.MaterialProperty.MP_NORMAL,"RGB",unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)]):
        node=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample,-400,i*180);node.texture=textures[suffix];node.set_editor_property("sampler_type",sampler)
        unreal.MaterialEditingLibrary.connect_material_property(node,channel,prop)
    metal=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-400,800);metal.r=.15;unreal.MaterialEditingLibrary.connect_material_property(metal,"",unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(material);lib.save_loaded_asset(material)
    cyan=unreal.load_asset("/Game/Units/Buildings/PolymerPlant/Materials/M_Polymer_Cyan");assert cyan
    for i,slot in enumerate(mesh.get_editor_property("static_materials")):
        mesh.set_material(i,cyan if "_Cyan" in str(slot.material_slot_name) else material)
    lib.save_loaded_asset(mesh)
    bounds=mesh.get_bounding_box()
    for before,after in [(old_bounds.min,bounds.min),(old_bounds.max,bounds.max)]:
        assert (before-after).length()<.25,(kind,before,after)
    size=bounds.max-bounds.min
    report[kind]={"size_cm":[size.x,size.y,size.z],"bounds_preserved":True,"material":material.get_path_name(),"textures":list(textures),"mesh":mesh.get_path_name()}
(root/"Saved/IndustrialBasecampStyleImport.json").write_text(json.dumps(report,indent=2))
unreal.log("BASECAMP_STYLE_IMPORT_COMPLETE "+json.dumps(report))
