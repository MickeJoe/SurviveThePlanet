import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
try:
    original=(root/'Scripts/test_mining_placement.py').read_text()
    original=original.replace('for resource_name, mine_path in [','for resource_name, mine_path in [\n    ("BP_CoalSource", "/Game/BluePrints/CoalMine/BP_CoalMine.BP_CoalMine_C"),')
    exec(compile(original,'test_mining_placement_with_coal.py','exec'))
    data=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CoalMine')
    copper=unreal.load_asset('/Game/Data/ResourceBuildings/DA_CopparMiningQuerry')
    for prop in ['construction_costs','max_drone_slots','initially_unlocked_drone_slots','energy_consumption_per_minute']:
        assert data.get_editor_property(prop)==copper.get_editor_property(prop),prop
    assert list(data.get_editor_property('supported_resource_types'))==[unreal.ResourceType.COAL]
    assert data.get_editor_property('output_per_drone_at100_percent')[0].amount_per_minute_per_drone_at100_percent==10
    report={'passed':True,'resources':['Coal','Stone','Copper','Iron'],'checks':['preview','building obstruction','attach','reservation','duplicate rejection','matching balance']}
except Exception as error:
    report={'passed':False,'error':str(error)}
    raise
finally:
    (root/'Saved/CoalPlacementTest.json').write_text(json.dumps(report,indent=2))
