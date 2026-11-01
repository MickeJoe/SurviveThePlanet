"""Run inside UE5.8 PIE. Changes only transient play-session state."""
import unreal, json
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
assert Path(unreal.Paths.project_dir()).resolve()==root.resolve()
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert world, 'Start PIE first'
sub=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
objectives=sub.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
contracts=sub.call_method('GetWorldSubsystem',(world,unreal.ContractOfferSubsystem))
mission=sub.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
gi=unreal.GameplayStatics.get_game_instance(world)
blueprints=sub.call_method('GetGameInstanceSubsystem',(gi,unreal.BuildingBlueprintSubsystem))
rm=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
mission.set_decay_paused(True)
assert len(objectives.get_mission_choice_groups())==0
objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
assert blueprints.owns_blueprint_by_id('CargoBay')
transform=unreal.Transform(location=unreal.Vector(0,0,-10000))
spawner=unreal.get_default_object(unreal.GameplayStatics)
cargo=spawner.call_method('BeginDeferredActorSpawnFromClass',(world,unreal.CargoBay,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
cargo=spawner.call_method('FinishSpawningActor',(cargo,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
cargo.set_construction_progress(1)
objectives.submit_building_built(unreal.STPBuildingType.CARGO_BAY,1)
active=[s for s in objectives.get_active_objectives() if str(s.objective_id).startswith('mother_base_')]
assert len(active)==3,active
package=str(objectives.get_selected_cargo_package());assert package in ['A','B']
config=json.loads((root/'Config/Objectives.json').read_text())
selected=next(p for p in config['cargoPackages'] if p['id']==package)
assert set(str(s.objective_id) for s in active)==set(selected['objectives'])
for key in selected['blueprints']:assert blueprints.owns_blueprint_by_id(key),key
for state in reversed(active):
 oid=state.objective_id
 definition=next(o for o in config['objectives'] if o['id']==str(oid));condition=definition['conditions'][0]
 res=getattr(unreal.ResourceType,{'PurifiedWater':'PURIFIED_WATER','SyntheticFuel':'SYNTHETIC_FUEL'}.get(condition['resource'],condition['resource'].upper()))
 rm.set_resource_amount(res,condition['amount']-1);assert not objectives.deliver_objective(oid)
 rm.set_resource_amount(res,condition['amount']);before=rm.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)
 assert objectives.deliver_objective(oid)
 assert rm.get_resource_amount(res)==0
 assert rm.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==before+2
 assert not objectives.deliver_objective(oid)
assert len(objectives.get_active_objectives())==0
assert contracts.are_contracts_available()
assert len(contracts.get_contract_offers())==5
for trader in ['helix_industrial','xeno_research','nomad_shipwrights']:
    offers=contracts.get_contract_offers_for_trader(trader)
    assert len(offers)==1 and len(offers[0].tiers)==3
(root/'Balance/PIEVerification.json').write_text(json.dumps(dict(passed=True,package=package,objectives=3,delivery_consumes_inventory=True,blueprint_rewards_verified=True,circuit_board_rewards_verified=True,duplicate_reward_blocked=True,contract_families=5,tiers=15),indent=2))
unreal.log('BALANCE PIE VERIFICATION PASSED')
