"""Apply the first industry balance pass in the canonical UE5.8 project."""
import unreal,json,traceback
from pathlib import Path
project=Path('C:/UE5/SurviveThePlanet 5.8')
assert Path(unreal.Paths.project_dir()).resolve()==project.resolve()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def costs(pairs):
    result=[]
    for resource,amount in pairs:
        c=unreal.ResourceCost();c.resource=resource;c.cost=amount;result.append(c)
    return result
def signature(values):return [(str(c.resource),c.cost) for c in values]
try:
    mine=[(unreal.ResourceType.IRON,10)]
    workshop=[(unreal.ResourceType.IRON,40)]
    communication=[(unreal.ResourceType.IRON,20),(unreal.ResourceType.CONCRETE,10)]
    targets=[('/Game/Data/Buildings/DA_MiningMachine',mine),('/Game/Data/Buildings/DA_CommunicationModel',communication),('/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop',workshop)]
    targets += [('/Game/Data/ResourceBuildings/'+n,mine) for n in ['DA_IronMiningQuerry','DA_StoneMiningQuerry','DA_CopparMiningQuerry','DA_CoalMine']]
    changed=[]
    for path,pairs in targets:
        asset=unreal.load_asset(path);assert asset,path
        asset.set_editor_property('construction_costs',costs(pairs))
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
        changed.append(path)
    catalog=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog')
    defs={a.get_editor_property('build_tool'):a for a in catalog.get_editor_property('buildings')}
    checks=[]
    classes=[('/Game/BluePrints/IronMiningQuerry/BP_MiningQuerry',mine),('/Game/BluePrints/StoneQuery/BP_StoneQuerry',mine),('/Game/BluePrints/CopparMiningQuerry/BP_CopparMiningQuerry',mine),('/Game/BluePrints/CoalMine/BP_CoalMine',mine),('/Game/BluePrints/MiningMachine/BP_MiningMachine',mine)]
    for tool,pairs in [(unreal.STPBuildTool.CRUDE_WORKSHOP,workshop),(unreal.STPBuildTool.COMMUNICATION_MODULE,communication)]:
        cls=defs[tool].get_editor_property('building_class')
        classes.append((cls.get_path_name().split('.')[0],pairs))
    for path,pairs in classes:
        bp=unreal.load_asset(path);assert bp,path
        default=unreal.get_default_object(bp.generated_class())
        if signature(default.get_construction_costs())!=signature(costs(pairs)):
            data=default.get_building_data()
            target=data if data else default
            target.set_editor_property('construction_costs',costs(pairs))
            if data:assert unreal.EditorAssetLibrary.save_loaded_asset(data,False)
            else:
                unreal.BlueprintEditorLibrary.compile_blueprint(bp)
                assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
                default=unreal.get_default_object(bp.generated_class())
        assert signature(default.get_construction_costs())==signature(costs(pairs)),path
        checks.append(dict(blueprint=path,effective_costs=signature(default.get_construction_costs())))
    for name in ['Nexaris','Aridus','Borealis']:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_'+name)
        managers=[a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a,unreal.ResourceManager)]
        assert managers
        for manager in managers:
            amounts=manager.get_editor_property('initial_resource_amounts')
            assert amounts[unreal.ResourceType.IRON]==20 and amounts[unreal.ResourceType.CONCRETE]==0,name
        checks.append(dict(planet=name,start_iron=20,start_concrete=0))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Planet/Maps/L_PlanetSelection')
    result=dict(passed=True,assets=changed,checks=checks)
except:result=dict(passed=False,error=traceback.format_exc())
(project/'Saved/BootstrapCostsVerification.json').write_text(json.dumps(result,indent=2))
