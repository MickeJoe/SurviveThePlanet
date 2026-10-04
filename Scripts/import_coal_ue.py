import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
src=root/'ContentSource'/'Coal';dest='/Game/Units/Buildings/Coal'
tools=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.EditorAssetLibrary
def imported(name,folder,texture=False):
    existing=unreal.load_asset(folder+'/'+name)
    if existing:return existing
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
palette={'Coal_Charcoal':((.035,.045,.055),.15,.38),'Coal_Fracture':((.105,.12,.135),.12,.48),'Coal_Sandstone':((.24,.17,.105),0,.6),'Coal_Steel':((.23,.28,.31),.75,.38),'Coal_Chassis':((.075,.09,.105),.65,.5),'Coal_Armor':((.63,.61,.54),.35,.48),'Coal_SafetyYellow':((.94,.58,.035),.2,.45),'Coal_StatusCyan':((.015,.55,.75),.1,.35)}
materials={}
for key,(color,metal,rough) in palette.items():
    path=dest+'/Materials/M_'+key;m=unreal.load_asset(path) or tools.create_asset('M_'+key,dest+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
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
source_data=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CopparMiningQuerry');assert source_data
data=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CoalMine') or tools.duplicate_asset('DA_CoalMine','/Game/Data/ResourceBuildings',source_data)
data.set_editor_property('display_name','Coal Mine');data.set_editor_property('description','Extracts coal from nearby deposits. Requires power and worker drones to operate.')
data.set_editor_property('building_mesh',meshes['SM_CoalMine']);data.set_editor_property('thumbnail',thumb);data.set_editor_property('toolbar_icon',thumb)
data.set_editor_property('supported_resource_types',[unreal.ResourceType.COAL]);rate=unreal.STPResourceOutputRate();rate.resource=unreal.ResourceType.COAL;rate.amount_per_minute_per_drone_at100_percent=10
data.set_editor_property('output_per_drone_at100_percent',[rate]);data.set_editor_property('source_transform_offset',unreal.Transform())
data.set_editor_property('blueprint_id','CoalMine');data.set_editor_property('show_in_build_toolbar',False)
factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.CoalMiningMachine)
bp=unreal.load_asset('/Game/BluePrints/CoalMine/BP_CoalMine') or tools.create_asset('BP_CoalMine','/Game/BluePrints/CoalMine',unreal.Blueprint,factory)
cdo=unreal.get_default_object(bp.generated_class());cdo.set_editor_property('building_data',data)
for comp in cdo.get_components_by_class(unreal.StaticMeshComponent):
    if comp.get_name()=='BuildingMesh':comp.set_static_mesh(meshes['SM_CoalMine'])
    elif comp.get_name()=='CoalCutter':comp.set_static_mesh(meshes['SM_CoalCutter'])
    elif comp.get_name() in ['ConveyorCoal','CuttingChips']:comp.set_static_mesh(meshes['SM_CoalChip'])
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
data.set_editor_property('building_class',bp.generated_class());lib.save_loaded_asset(data);lib.save_loaded_asset(bp)
source=unreal.load_asset('/Game/BluePrints/Resources/BP_CoalSource') or tools.duplicate_asset('BP_CoalSource','/Game/BluePrints/Resources',unreal.load_asset('/Game/BluePrints/Resources/BP_StoneSource'))
source_cdo=unreal.get_default_object(source.generated_class());source_cdo.set_editor_property('resource_type',unreal.ResourceType.COAL);source_cdo.set_editor_property('mine_blueprint',bp.generated_class());source_cdo.set_editor_property('starting_amount',4000)
for comp in source_cdo.get_components_by_class(unreal.StaticMeshComponent):
    if comp.get_name()=='ResourceMesh':
        comp.set_static_mesh(meshes['SM_CoalDeposit']);comp.set_editor_property('relative_scale3d',unreal.Vector(1,1,1));comp.set_editor_property('relative_location',unreal.Vector())
        comp.set_editor_property('relative_rotation',unreal.Rotator());comp.set_editor_property('override_materials',[])
unreal.BlueprintEditorLibrary.compile_blueprint(source);lib.save_loaded_asset(source)
# The general Mining Machine toolbar remains authoritative; a source chooses its specific mine.
generic=unreal.load_asset('/Game/Data/Buildings/DA_MiningMachine')
types=list(generic.get_editor_property('supported_resource_types'))
if unreal.ResourceType.COAL not in types:types.append(unreal.ResourceType.COAL)
generic.set_editor_property('supported_resource_types',types)
rates=list(generic.get_editor_property('output_per_drone_at100_percent'))
if not any(r.resource==unreal.ResourceType.COAL for r in rates):rates.append(rate)
generic.set_editor_property('output_per_drone_at100_percent',rates);lib.save_loaded_asset(generic)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
current=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]
maps=[current,'/Game/PlanetLevel']
changed=[]
for map_path in dict.fromkeys(maps):
    if not lib.does_asset_exist(map_path):continue
    if map_path!=current:assert levels.load_level(map_path)
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.SectorPopulation):
            classes=dict(a.get_editor_property('deposit_classes'));classes[unreal.ResourceType.COAL]=source.generated_class();a.set_editor_property('deposit_classes',classes)
            changed.append(map_path)
    assert levels.save_current_level()
if current!=maps[-1]:levels.load_level(current)
(root/'Saved'/'CoalImport.json').write_text(json.dumps({'passed':True,'maps':changed,'mesh_bounds':{k:str(v.get_bounding_box()) for k,v in meshes.items()},'data':data.get_path_name()},indent=2))
unreal.log('COAL_IMPORT_COMPLETE')
