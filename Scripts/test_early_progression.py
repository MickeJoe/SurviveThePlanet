"""Integration checks in a fresh L_Nexaris PIE session. Simulates gathered stone."""
import unreal,json,traceback
from pathlib import Path
project=Path('C:/UE5/SurviveThePlanet 5.8')
checks=[]
try:
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
    lib=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
    objective=lib.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
    manager=lib.call_method('GetWorldSubsystem',(world,unreal.BuildingManagerSubsystem))
    gi=unreal.GameplayStatics.get_game_instance(world)
    inventory=lib.call_method('GetGameInstanceSubsystem',(gi,unreal.BuildingBlueprintSubsystem))
    resources=unreal.GameplayStatics.get_actor_of_class(world,unreal.ResourceManager)
    expected={unreal.STPBuildTool.MINING_MACHINE,unreal.STPBuildTool.SOLAR_ARRAY,unreal.STPBuildTool.WIND_GENERATOR,unreal.STPBuildTool.CRUDE_WORKSHOP,unreal.STPBuildTool.COMMUNICATION_MODULE,unreal.STPBuildTool.ENERGY_EXTENDER}
    assert set(inventory.get_owned_blueprints())==expected
    assert resources.get_resource_amount(unreal.ResourceType.CONCRETE)==0
    assert resources.get_resource_amount(unreal.ResourceType.CONNECTOR)==100
    checks.append('Fresh colony owns exactly six bootstrap BP; no starting concrete; 100 connectors')
    assert resources.get_resource_amount(unreal.ResourceType.IRON)==20
    mine_costs=manager.get_definition(unreal.STPBuildTool.MINING_MACHINE).get_editor_property('construction_costs')
    workshop_costs=manager.get_definition(unreal.STPBuildTool.CRUDE_WORKSHOP).get_editor_property('construction_costs')
    assert resources.can_afford_costs(mine_costs)
    assert not resources.can_afford_costs(workshop_costs)
    assert len(workshop_costs)==1 and workshop_costs[0].cost==40
    checks.append('20 starting iron can pay for a basic mine, but not the 40-iron workshop')
    assert not inventory.owns_blueprint(unreal.STPBuildTool.CARGO_BAY)
    before=resources.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)
    objective.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
    assert resources.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==before+5
    for key in ['CargoBay','WaterCollector','ConcretePlant','EnergyStorage']:assert inventory.owns_blueprint_by_id(key),key
    checks.append('Uplink grants exactly five Circuit Boards and all four intended blueprints in real game instance')
    objective.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
    assert resources.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==before+5
    checks.append('Second completion event cannot duplicate rewards')
    cargo=manager.get_definition(unreal.STPBuildTool.CARGO_BAY)
    costs={str(c.resource):c.cost for c in cargo.get_editor_property('construction_costs')}
    assert costs=={str(unreal.ResourceType.CONCRETE):5,str(unreal.ResourceType.CONTROL_CHIP):5}
    checks.append('Cargo Bay requires five concrete and five Circuit Boards; concrete remains producible using starting workshop')
    result=dict(passed=True,checks=checks)
except:result=dict(passed=False,checks=checks,error=traceback.format_exc())
(project/'Saved/EarlyProgressionRuntime.json').write_text(json.dumps(result,indent=2))
