"""Run in UE5.8, outside PIE, after loading L_Nexaris."""
import unreal, json
from pathlib import Path

PROJECT=Path('C:/UE5/SurviveThePlanet 5.8')
assert Path(unreal.Paths.project_dir()).resolve() == PROJECT.resolve()
START={unreal.STPBuildTool.SOLAR_ARRAY,unreal.STPBuildTool.WIND_GENERATOR,
       unreal.STPBuildTool.ENERGY_EXTENDER,unreal.STPBuildTool.MINING_MACHINE,
       unreal.STPBuildTool.CRUDE_WORKSHOP,unreal.STPBuildTool.COMMUNICATION_MODULE}
catalog=unreal.load_asset('/Game/Data/Buildings/DA_BuildingCatalog')
def cost(resource,amount):
    value=unreal.ResourceCost(); value.resource=resource; value.cost=amount
    return value
changed=[]
for asset in catalog.get_editor_property('buildings'):
    tool=asset.get_editor_property('build_tool')
    owned=tool in START
    dirty=asset.get_editor_property('blueprint_initially_owned') != owned
    asset.set_editor_property('blueprint_initially_owned',owned)
    if tool == unreal.STPBuildTool.MINING_MACHINE:
        asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.IRON,10)]);dirty=True
    elif tool == unreal.STPBuildTool.CRUDE_WORKSHOP:
        asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.IRON,40)]);dirty=True
    elif tool == unreal.STPBuildTool.COMMUNICATION_MODULE:
        asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.IRON,20),cost(unreal.ResourceType.CONCRETE,10)]);dirty=True
    elif tool == unreal.STPBuildTool.CARGO_BAY:
        asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.CONCRETE,20),cost(unreal.ResourceType.STEEL,10),cost(unreal.ResourceType.CONTROL_CHIP,2)]);dirty=True
    elif tool in [unreal.STPBuildTool.SOLAR_ARRAY,unreal.STPBuildTool.WIND_GENERATOR]:
        asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.IRON,10 if tool==unreal.STPBuildTool.SOLAR_ARRAY else 15)]);dirty=True
    if dirty:
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False), asset.get_path_name()
        changed.append(asset.get_path_name())

# Resource-specific mines use their own definitions rather than the toolbar definition.
for name in ['DA_IronMiningQuerry','DA_StoneMiningQuerry','DA_CopparMiningQuerry','DA_CoalMine']:
    asset=unreal.load_asset('/Game/Data/ResourceBuildings/'+name)
    assert asset,name
    asset.set_editor_property('construction_costs',[cost(unreal.ResourceType.IRON,10)])
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)

# Keep the shared Resource Manager default consistent for all landing maps.
resource_bp=unreal.load_asset('/Game/BluePrints/Resources/BP_ResourceManager')
resource_defaults=unreal.get_default_object(resource_bp.generated_class())
default_amounts=dict(resource_defaults.get_editor_property('initial_resource_amounts'))
default_amounts[unreal.ResourceType.CONCRETE]=0
default_amounts[unreal.ResourceType.IRON]=20
resource_defaults.set_editor_property('initial_resource_amounts',default_amounts)
assert unreal.EditorAssetLibrary.save_loaded_asset(resource_bp,False)

for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,unreal.ResourceManager):
        amounts=dict(actor.get_editor_property('initial_resource_amounts'))
        amounts[unreal.ResourceType.CONCRETE]=0
        amounts[unreal.ResourceType.IRON]=20
        amounts[unreal.ResourceType.CONNECTOR]=100
        actor.set_editor_property('initial_resource_amounts',amounts)
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

path=PROJECT/'Config/Objectives.json'
data=json.loads(path.read_text(encoding='utf-8-sig'))
objective=next(o for o in data['objectives'] if o['id']=='mission_build_communication')
objective['description']='Mine stone and use Crude Workshop to produce 10 Concrete. Spend 20 Iron and 10 Concrete to build the Communication Module to contact Mother Base. Rewards: 5 Circuit Boards; Cargo Bay, Water Collector, Concrete Plant and Energy Storage blueprints; 1 Explorer Drone and 1 Construction Drone; +40 Mission Confidence; +1 Mission Progression.'
for reward in objective['rewards']:
    if reward['type']=='give_resource' and reward.get('resource')=='ControlChip': reward['amount']=5
objective['rewards']=[r for r in objective['rewards'] if r['type']!='give_building_blueprint']
for blueprint in ['CargoBay','WaterCollector','ConcretePlant','EnergyStorage']:
    objective['rewards'].append(dict(type='give_building_blueprint',blueprintId=blueprint))
path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
(PROJECT/'Saved/EarlyProgressionAuthoring.json').write_text(json.dumps(dict(changed_assets=changed,start_blueprints=sorted(str(x) for x in START)),indent=2))
unreal.log('Early progression configured and saved.')
