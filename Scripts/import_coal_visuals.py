import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
src=root/'ContentSource'/'Coal';dest='/Game/Units/Buildings/Coal'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
def imported(name,folder,texture=False):
    existing=unreal.load_asset(folder+'/'+name)
    if existing and name in ['SM_CoalDeposit','T_Coal']:return existing
    task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')));task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
    if not texture:
        opts=unreal.FbxImportUI();opts.import_mesh=True;opts.import_materials=True;opts.import_textures=False;opts.import_as_skeletal=False
        opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.generate_lightmap_u_vs=True;opts.static_mesh_import_data.auto_generate_collision=True
        task.options=opts
    tools.import_asset_tasks([task]);asset=unreal.load_asset(folder+'/'+name);assert asset,name;return asset
meshes={n:imported(n,dest+'/Meshes') for n in ['SM_CoalDeposit','SM_CoalMine','SM_CoalCutter','SM_CoalChip']}
icon=imported('T_Coal','/Game/UI/Icons',texture=True);thumb=imported('T_CoalMine','/Game/UI/Icons/Buildings',texture=True)
for tex in [icon,thumb]:
    tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);lib.save_loaded_asset(tex)
palette={'Coal_Charcoal':((.035,.045,.055),.15,.38),'Coal_Fracture':((.105,.12,.135),.12,.48),'Coal_Sandstone':((.24,.17,.105),0,.6),'Coal_Steel':((.23,.28,.31),.75,.38),'Coal_Chassis':((.075,.09,.105),.65,.5),'Coal_Armor':((.40,.365,.29),.35,.58),'Coal_SafetyYellow':((.94,.58,.035),.2,.45),'Coal_StatusCyan':((.015,.55,.75),.1,.35)}
materials={}
for key,(color,metal,rough) in palette.items():
    path=dest+'/Materials/M_'+key;m=unreal.load_asset(path) or tools.create_asset('M_'+key,dest+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    if key in ['Coal_Charcoal','Coal_Fracture','Coal_Sandstone']:
        materials[key]=m;continue
    unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
    c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-300,0);c.constant=unreal.LinearColor(*color,1)
    unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for val,prop,y in [(metal,unreal.MaterialProperty.MP_METALLIC,100),(rough,unreal.MaterialProperty.MP_ROUGHNESS,200)]:
        n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-300,y);n.r=val;unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
    if key=='Coal_StatusCyan':unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(m);lib.save_loaded_asset(m);materials[key]=m
for mesh in meshes.values():
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        slotname=str(slot.material_slot_name)
        key=next((k for k in palette if k in slotname),None)
        if key:mesh.set_material(i,materials[key])
    lib.save_loaded_asset(mesh)


exec((root/'Scripts/finish_coal_ue_materials.py').read_text())
exec((root/'Scripts/finish_coal_dust_usage.py').read_text())
blueprint=unreal.load_asset('/Game/BluePrints/CoalMine/BP_CoalMine')
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
lib.save_loaded_asset(blueprint,False)
(root/'Saved/CoalVisualImport.json').write_text(json.dumps({'passed':True,'reimported_visuals':True},indent=2))
unreal.log('COAL_VISUAL_IMPORT_PASSED')

exec((root/'Scripts/apply_coal_size_palette.py').read_text())
