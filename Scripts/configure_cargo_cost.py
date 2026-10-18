import unreal,json
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
catalog=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog')
cargo=[a for a in catalog.get_editor_property('buildings') if a.get_editor_property('build_tool')==unreal.STPBuildTool.CARGO_BAY]
assert len(cargo)==1
costs=[]
for resource,amount in [(unreal.ResourceType.CONCRETE,20),(unreal.ResourceType.STEEL,10),(unreal.ResourceType.CONTROL_CHIP,2)]:
    c=unreal.ResourceCost();c.resource=resource;c.cost=amount;costs.append(c)
cargo[0].set_editor_property('construction_costs',costs)
assert unreal.EditorAssetLibrary.save_loaded_asset(cargo[0],False)
cls=cargo[0].get_editor_property('building_class');assert cls
default=unreal.get_default_object(cls)
target=default.get_building_data() or default
target.set_editor_property('construction_costs',costs)
if target!=default:assert unreal.EditorAssetLibrary.save_loaded_asset(target,False)
else:
    bp=unreal.load_asset(cls.get_path_name().split('.')[0]);assert isinstance(bp,unreal.Blueprint)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
assert {c.resource:c.cost for c in unreal.get_default_object(cls).get_construction_costs()}=={c.resource:c.cost for c in costs}
(taskdir/'cargo_cost_authoring.json').write_text(json.dumps({'asset':cargo[0].get_path_name(),'costs':{'Concrete':20,'Steel':10,'ControlChip':2}},indent=2))
