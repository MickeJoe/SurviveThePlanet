import unreal,json,traceback
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
import time
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
sub=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
contracts=sub.call_method('GetWorldSubsystem',(world,unreal.ContractOfferSubsystem))
mission=sub.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
objectives=sub.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
manager=sub.call_method('GetWorldSubsystem',(world,unreal.BuildingManagerSubsystem))
wl=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
panels=wl.call_method('GetAllWidgetsOfClass',(world,unreal.TraderPanelWidget,False));assert panels;panel=panels[0]
assert not contracts.are_contracts_available()
assert not contracts.get_contract_offers_for_trader('helix_industrial')
assert not contracts.toggle_contract_selection('helix_advanced_components','standard')
assert not contracts.confirm_selected_contract()
assert panel.get_visibility()==unreal.SlateVisibility.COLLAPSED
mission.set_decay_paused(True);mission.set_mission_confidence(20)
objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
assert mission.get_mission_confidence()==60,mission.get_mission_confidence()
assert not contracts.are_contracts_available()
assert panel.get_visibility()==unreal.SlateVisibility.COLLAPSED
data=manager.get_definition(unreal.STPBuildTool.CARGO_BAY)
costs={str(c.resource):c.cost for c in data.get_editor_property('construction_costs')}
assert sorted(costs.values())==[2,10,20],costs
expected={unreal.ResourceType.CONCRETE:20,unreal.ResourceType.STEEL:10,unreal.ResourceType.CONTROL_CHIP:2}
assert {c.resource:c.cost for c in data.get_editor_property('construction_costs')}==expected
statics=unreal.get_default_object(unreal.GameplayStatics)
t=unreal.Transform(location=unreal.Vector(0,0,-10000));cls=manager.get_building_class(unreal.STPBuildTool.CARGO_BAY)
cargo=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
cargo=statics.call_method('FinishSpawningActor',(cargo,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
assert {c.resource:c.cost for c in cargo.get_construction_costs()}==expected
cargo.set_construction_progress(.5)
assert not contracts.are_contracts_available()
cargo.set_construction_progress(1)
assert contracts.are_contracts_available()
def check_cargo_ui(dt):
    try:
        assert panel.get_visibility()!=unreal.SlateVisibility.COLLAPSED,panel.get_visibility()
        assert contracts.get_contract_offers_for_trader('helix_industrial')
        assert contracts.toggle_contract_selection('helix_advanced_components','standard')
        assert contracts.confirm_selected_contract()
        (taskdir/'cargo_gate_test.json').write_text(json.dumps({'passed':True,'uplink_confidence_reward':40,'cargo_costs':{'Concrete':20,'Steel':10,'ControlChip':2},'locked_before_cargo':True,'locked_after_uplink':True,'locked_during_construction':True,'panel_shown_after_completion':True,'contract_selection_and_confirmation':True},indent=2))
    except:(taskdir/'cargo_test_error.txt').write_text(traceback.format_exc())
    unreal.unregister_slate_post_tick_callback(cargo_ui_handle)
cargo_ui_handle=unreal.register_slate_post_tick_callback(check_cargo_ui)
