import unreal,json
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
import time
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
sub=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
mission=sub.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem))
objectives=sub.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
mission.set_decay_paused(True)
assert abs(mission.get_decay_per_game_hour()-2.5)<0.001
builder_class=unreal.load_asset('/Game/BluePrints/WorkingDrone/BP_ConstructionDrone').generated_class()
builders=actors(builder_class);explorers=actors(unreal.ExplorerDrone)
assert len(builders)==2,len(builders)
assert len(actors(unreal.BaseDrone))==3,len(actors(unreal.BaseDrone))
assert len(explorers)==0,len(explorers)
rm=actors(unreal.ResourceManager)[0];chips=rm.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)
mission.set_mission_confidence(20)
objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
assert abs(mission.get_mission_confidence()-60)<0.001,mission.get_mission_confidence()
assert len(actors(builder_class))==3
assert len(actors(unreal.ExplorerDrone))==1
assert rm.get_resource_amount(unreal.ResourceType.CONTROL_CHIP)==chips+5
objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
assert len(actors(builder_class))==3 and len(actors(unreal.ExplorerDrone))==1
assert abs(mission.get_mission_confidence()-60)<0.001
workshop=unreal.get_default_object(unreal.CrudeWorkshop)
assert abs(workshop.get_cycle_seconds()-20)<0.001,workshop.get_cycle_seconds()
(taskdir/'balance_round2_test.json').write_text(json.dumps({'passed':True,'start_total_drones':3,'start_builders':2,'after_uplink_builders':3,'after_uplink_explorers':1,'confidence_before':20,'confidence_after':60,'decay_per_hour':mission.get_decay_per_game_hour(),'concrete_cycle':workshop.get_cycle_seconds(),'reward_duplicate_prevented':True,'circuit_board_reward':5},indent=2))
