"""Import the Blender ships, bind each merchant and refine the existing WBPs."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
assert not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary;mel=unreal.MaterialEditingLibrary
source=root/'ContentSource/Trading/Ships';destination='/Game/Units/Trading/Ships'
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
def imported(name,src,folder,texture=False):
    task=unreal.AssetImportTask();task.filename=str(src/(name+('.png' if texture else '.fbx')))
    task.destination_path=folder;task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;task.replace_existing_settings=True
    if not texture:
        existing=unreal.load_asset(folder+'/'+name)
        if existing:
            data=existing.get_editor_property('asset_import_data')
            data.set_editor_property('import_uniform_scale',1.0);data.set_editor_property('convert_scene_unit',False)
            lib.save_loaded_asset(existing)
        task.factory=unreal.FbxFactory();options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False
        options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False
        options.static_mesh_import_data.generate_lightmap_u_vs=True;options.static_mesh_import_data.transform_vertex_to_absolute=False
        options.static_mesh_import_data.bake_pivot_in_vertex=False;options.static_mesh_import_data.convert_scene_unit=False
        options.static_mesh_import_data.import_uniform_scale=1.0
        task.options=options
    tools.import_asset_tasks([task]);asset=unreal.load_asset(folder+'/'+name);assert asset,name
    if not texture:
        # FBX vertices are exported in centimetres with separate part pivots.
        editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        settings=editor.get_lod_build_settings(asset,0)
        settings.set_editor_property('build_scale3d',unreal.Vector(1,1,1))
        editor.set_lod_build_settings(asset,0,settings)
        lib.save_loaded_asset(asset)
    return asset
def material(name,folder):
    m=unreal.load_asset(folder+'/'+name) or tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m);return m
def node(m,cls):return mel.create_material_expression(m,cls)
def value(m,v):
    n=node(m,unreal.MaterialExpressionConstant);n.r=v;return n
def color(m,rgb):
    n=node(m,unreal.MaterialExpressionConstant3Vector);n.constant=unreal.LinearColor(*rgb,1);return n
def link(a,b,pin,output=''):assert mel.connect_material_expressions(a,output,b,pin)
def prop(n,p,output=''):assert mel.connect_material_property(n,output,p)
def parameter(m,name,default):
    n=node(m,unreal.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',default);return n
def multiply(m,a,b):
    n=node(m,unreal.MaterialExpressionMultiply);link(a,n,'A');link(b,n,'B');return n
shared=destination+'/Shared'
cyan=material('M_MerchantNavigation',shared)
prop(color(cyan,(.015,.55,.80)),unreal.MaterialProperty.MP_BASE_COLOR)
prop(color(cyan,(.025,1.4,2.2)),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
prop(value(cyan,.35),unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(cyan);lib.save_loaded_asset(cyan)
heat=material('M_MerchantBeacon',shared)
prop(color(heat,(1,.23,.035)),unreal.MaterialProperty.MP_BASE_COLOR)
prop(color(heat,(2,.46,.07)),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(heat);lib.save_loaded_asset(heat)
records=json.loads((source/'MerchantShips.json').read_text());report=[]
for record in records:
    key=record['merchant'];folder=destination+'/'+key;src=source/key
    texture=imported('T_'+key+'_BaseColor',src,folder+'/Textures',True)
    surface=material('M_'+key+'_Hull',folder+'/Materials')
    sample=node(surface,unreal.MaterialExpressionTextureSample);sample.texture=texture
    prop(sample,unreal.MaterialProperty.MP_BASE_COLOR,'RGB');prop(value(surface,.35),unreal.MaterialProperty.MP_METALLIC)
    prop(value(surface,.62),unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(surface);lib.save_loaded_asset(surface)
    meshes={}
    for part in ['Body','Gear','Ramp']:
        mesh=imported('SM_'+key+'_'+part,src,folder+'/Meshes')
        for index,slot in enumerate(mesh.get_editor_property('static_materials')):
            name=str(slot.material_slot_name)
            mesh.set_material(index,cyan if '_Cyan' in name else heat if '_Heat' in name else surface)
        lib.save_loaded_asset(mesh);meshes[part]=mesh
    bounds=meshes['Body'].get_bounding_box();size=bounds.max-bounds.min
    assert 300 < max(size.x,size.y) < 500,(key,str(size))
    merchant=unreal.load_asset('/Game/Data/Trading/Merchants/DA_'+key);assert merchant
    merchant.set_editor_property('ship_mesh',meshes['Body']);merchant.set_editor_property('landing_gear_mesh',meshes['Gear'])
    merchant.set_editor_property('loading_ramp_mesh',meshes['Ramp'])
    merchant.set_editor_property('loading_ramp_pivot',unreal.Vector(*record['ramp_pivot_cm']))
    merchant.set_editor_property('thruster_locations',[unreal.Vector(*v) for v in record['thrusters_cm']])
    merchant.set_editor_property('ship_scale',1.0)
    lib.save_loaded_asset(merchant)
    report.append({'merchant':key,'size_cm':str(size),'body_bounds':str(bounds),'gear_bounds':str(meshes['Gear'].get_bounding_box()),'ramp_bounds':str(meshes['Ramp'].get_bounding_box())})
falloff=imported('T_MerchantFXFalloff',source,shared,True)
falloff.set_editor_property('srgb',False);lib.save_loaded_asset(falloff)
for name,is_dust in [('M_MerchantExhaust',False),('M_MerchantDust',True)]:
    m=material(name,shared);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT if is_dust else unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('two_sided',True)
    opacity=node(m,unreal.MaterialExpressionTextureSample);opacity.texture=falloff
    opacity.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    strength=parameter(m,'Strength' if is_dust else 'Thrust',0)
    alpha=node(m,unreal.MaterialExpressionMultiply);link(opacity,alpha,'A','A');link(strength,alpha,'B')
    prop(alpha,unreal.MaterialProperty.MP_OPACITY)
    if is_dust:
        prop(color(m,(.23,.17,.10)),unreal.MaterialProperty.MP_BASE_COLOR);prop(value(m,1),unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        prop(multiply(m,color(m,(.05,2.0,3.5)),strength),unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m);lib.save_loaded_asset(m)
    mesh=imported('SM_MerchantDust' if is_dust else 'SM_MerchantExhaust',source,shared)
    mesh.set_material(0,m);lib.save_loaded_asset(mesh)
unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 1')

# Extend existing WBP assets without rebuilding or losing their authored frame.
from editor_toolset.toolsets.object import ObjectTools
helpers=(root/'Scripts/create_trade_ui.py').read_text(encoding='utf-8');exec(helpers[:helpers.index('credits=unreal.load_asset')])
bp=unreal.load_asset('/Game/UI/Development/WBP_CheatMenu')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
column=next(w for w in nodes.values() if isinstance(w,unreal.VerticalBox))
if 'Speed30Button' not in nodes:
    speed=add(bp,unreal.Button,'Speed30Button',column,True);props(speed,{'backgroundColor':TEAL,'isFocusable':False})
    props(speed.slot,{'padding':{'left':0,'top':8,'right':0,'bottom':0}})
    caption=text(bp,'Speed30ButtonLabel',speed,'SPEED x30',0,0,0,0,17,WHITE)
    props(caption.slot,{'horizontalAlignment':'HAlign_Center','verticalAlignment':'VAlign_Center','padding':{'left':8,'top':8,'right':8,'bottom':8}})
assert umg.call_method('CompileWidgetBlueprint',(bp,));assert lib.save_loaded_asset(bp)
bp=unreal.load_asset('/Game/UI/Trading/WBP_VisitingMerchant')
nodes={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}
place(nodes['ArrivalText'],96,80,222,25)
font=json.loads(ObjectTools.get_properties(nodes['ArrivalText'],['font']))['font'];font['size']=17
props(nodes['ArrivalText'],{'font':font});place(nodes['TradeButton'],96,108,222,27)
umg.call_method('ToggleWidgetAsVariable',(bp,nodes['TradeButtonLabel'],True))
assert umg.call_method('CompileWidgetBlueprint',(bp,));assert lib.save_loaded_asset(bp)
bay=unreal.load_asset('/Game/Models/Buildings/CargoBay/CargoBayMesh')
report.append({'cargo_bay_bounds':str(bay.get_bounding_box())})
(root/'Saved/MerchantShipImport.json').write_text(json.dumps(report,indent=2))
unreal.log('MERCHANT_SHIPS_IMPORTED '+json.dumps(report))
