import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir());assert (root/'SurviveThePlanet.uproject').exists()
palette={'Coal_Armor':'Extender_Ivory','Coal_Steel':'Extender_Steel','Coal_Chassis':'Extender_Graphite','Coal_SafetyYellow':'Extender_Orange','Coal_StatusCyan':'Extender_Cyan'}
for name in ['SM_CoalMine','SM_CoalCutter']:
    mesh=unreal.load_asset('/Game/Units/Buildings/Coal/Meshes/'+name)
    for index,slot in enumerate(mesh.get_editor_property('static_materials')):
        key=next((k for k in palette if k in str(slot.material_slot_name)),None)
        if key:
            material=unreal.load_asset('/Game/Units/Buildings/EnergyExtender/'+palette[key]);assert material
            mesh.set_material(index,material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
blueprint=unreal.load_asset('/Game/BluePrints/CoalMine/BP_CoalMine')
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
defaults=unreal.get_default_object(blueprint.generated_class())
mesh=next(c for c in defaults.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
mesh.set_editor_property('relative_scale3d',unreal.Vector(1.25,1.25,1.25))
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint,False)
task=unreal.AssetImportTask();task.filename=str(root/'ContentSource/Coal/T_CoalMine.png');task.destination_path='/Game/UI/Icons/Buildings';task.destination_name='T_CoalMine';task.automated=True;task.replace_existing=True;task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
report={'scale':str(mesh.get_editor_property('relative_scale3d')),'shared_palette':palette,'footprint':str(defaults.get_grid_footprint())}
# Confirm the default survives a package reload, rather than just the current CDO.
unreal.EditorLoadingAndSavingUtils.reload_packages([blueprint.get_outer()])
blueprint=unreal.load_asset('/Game/BluePrints/CoalMine/BP_CoalMine')
defaults=unreal.get_default_object(blueprint.generated_class())
mesh=next(c for c in defaults.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BuildingMesh')
assert mesh.get_editor_property('relative_scale3d')==unreal.Vector(1.25,1.25,1.25),'Scale was not saved in Blueprint'
report['reload_verified']=True
(root/'Saved/CoalSizePalette.json').write_text(json.dumps(report,indent=2))
unreal.log('COAL_SIZE_PALETTE_PASSED '+json.dumps(report))
