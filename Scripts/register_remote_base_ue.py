import unreal,json
from pathlib import Path
ROOT=Path(r'C:\UE5\SurviveThePlanet 5.8')
assert Path(unreal.Paths.project_dir()).resolve()==ROOT.resolve()
tools=unreal.AssetToolsHelpers.get_asset_tools()
folder='/Game/Units/Buildings/RemoteBase'
bp=unreal.load_asset(folder+'/BP_RemoteBaseBuilding')
if not bp:
    factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.RemoteBase)
    bp=tools.create_asset('BP_RemoteBaseBuilding',folder,unreal.Blueprint,factory)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
icon=unreal.load_asset(folder+'/T_RemoteBase')
if not icon:
    task=unreal.AssetImportTask()
    task.filename=str(ROOT/'ContentSource/RemoteBase/T_RemoteBase.png')
    task.destination_path=folder;task.destination_name='T_RemoteBase'
    task.automated=True;task.save=True;task.replace_existing=True
    tools.import_asset_tasks([task]);icon=unreal.load_asset(folder+'/T_RemoteBase')
assert icon
icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
icon.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
icon.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
catalog=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog')
entries=list(catalog.get_editor_property('buildings'))
extender=next(d for d in entries if d.get_editor_property('build_tool')==unreal.STPBuildTool.ENERGY_EXTENDER)
data=unreal.load_asset('/Game/Data/Buildings/DA_RemoteBase')
if not data:
    factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.BuildingDataAsset)
    data=tools.create_asset('DA_RemoteBase','/Game/Data/Buildings',unreal.BuildingDataAsset,factory)
properties={
    'build_tool':unreal.STPBuildTool.REMOTE_BASE,
    'building_type':unreal.STPBuildingType.REMOTE_BASE,
    'build_category':unreal.STPBuildCategory.INFRASTRUCTURE,
    'blueprint_id':unreal.Name('RemoteBase'),
    'blueprint_initially_owned':True,'show_in_build_toolbar':True,
    'toolbar_sort_order':max([d.get_editor_property('toolbar_sort_order') for d in entries if d.get_editor_property('build_category')==unreal.STPBuildCategory.INFRASTRUCTURE]+[0])+10,
    'building_class':bp.generated_class(),
    'display_name':unreal.Text('Remote Base'),
    'description':unreal.Text('Establishes a discovered sector for construction. Only one Base Camp or Remote Base per sector. Requires a connection to the power grid; connector cost increases with distance.'),
    'building_tag':unreal.Name('RemoteBase'),
    'building_mesh':unreal.load_asset(folder+'/Meshes/SM_RemoteBase_Body'),
    'toolbar_icon':icon,'thumbnail':icon,
    'connectors_per_meter':extender.get_editor_property('connectors_per_meter'),
    'construction_costs':[],'override_energy_settings':True,
    'energy_consumption_per_minute':0.0,'energy_production_per_minute':0.0,'energy_storage_capacity':0.0,
}
for name,value in properties.items():data.set_editor_property(name,value)
unreal.get_default_object(bp.generated_class()).set_editor_property('building_data',data)
unreal.get_default_object(bp.generated_class()).get_editor_property('energy_coverage').set_editor_property('coverage_radius',2200.0)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
if data not in entries:entries.append(data)
catalog.set_editor_property('buildings',entries)
for asset in [icon,data,bp,catalog]:unreal.EditorAssetLibrary.save_loaded_asset(asset)
report={'blueprint':bp.get_path_name(),'definition':data.get_path_name(),'toolbar_icon':icon.get_path_name(),'connectors_per_meter':data.get_editor_property('connectors_per_meter'),'catalog_count':len(entries)}
(ROOT/'Saved/RemoteBase/registration.json').write_text(json.dumps(report,indent=2))
unreal.log('REMOTE_BASE_REGISTERED '+json.dumps(report))


